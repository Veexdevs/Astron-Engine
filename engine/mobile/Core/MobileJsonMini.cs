using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace Astron.Engine.Mobile
{
    /// <summary>
    /// Parser e serializador JSON leve e determinístico compatível com netstandard2.0
    /// (zero dependências externas NuGet para compilar rápido em APK e IPA).
    /// </summary>
    public sealed class JsonVal
    {
        public enum Kind { Null, Bool, Number, String, Array, Object }

        public Kind Type { get; private set; }
        public bool BoolVal { get; private set; }
        public double NumVal { get; private set; }
        public string StrVal { get; private set; }
        public List<JsonVal> ArrVal { get; private set; }
        public Dictionary<string, JsonVal> ObjVal { get; private set; }

        public static JsonVal Null() => new JsonVal { Type = Kind.Null };
        public static JsonVal FromBool(bool b) => new JsonVal { Type = Kind.Bool, BoolVal = b };
        public static JsonVal FromNum(double n) => new JsonVal { Type = Kind.Number, NumVal = n };
        public static JsonVal FromStr(string s) => new JsonVal { Type = Kind.String, StrVal = s ?? string.Empty };

        public static JsonVal NewArray() => new JsonVal { Type = Kind.Array, ArrVal = new List<JsonVal>() };
        public static JsonVal NewObject() => new JsonVal { Type = Kind.Object, ObjVal = new Dictionary<string, JsonVal>() };

        public JsonVal this[string key]
        {
            get
            {
                if (Type == Kind.Object && ObjVal != null && ObjVal.TryGetValue(key, out JsonVal v))
                    return v;
                return Null();
            }
            set
            {
                if (Type != Kind.Object)
                {
                    Type = Kind.Object;
                    ObjVal = new Dictionary<string, JsonVal>();
                }
                ObjVal[key] = value ?? Null();
            }
        }

        public void Add(JsonVal item)
        {
            if (Type != Kind.Array)
            {
                Type = Kind.Array;
                ArrVal = new List<JsonVal>();
            }
            ArrVal.Add(item ?? Null());
        }

        public string AsString(string fallback = "") => Type == Kind.String ? (StrVal ?? fallback) : fallback;
        public double AsDouble(double fallback = 0.0) => Type == Kind.Number ? NumVal : fallback;
        public float AsFloat(float fallback = 0f) => Type == Kind.Number ? (float)NumVal : fallback;
        public int AsInt(int fallback = 0) => Type == Kind.Number ? (int)NumVal : fallback;
        public bool AsBool(bool fallback = false) => Type == Kind.Bool ? BoolVal : fallback;

        public string Dump()
        {
            var sb = new StringBuilder();
            WriteTo(sb);
            return sb.ToString();
        }

        private void WriteTo(StringBuilder sb)
        {
            switch (Type)
            {
                case Kind.Null:
                    sb.Append("null");
                    break;
                case Kind.Bool:
                    sb.Append(BoolVal ? "true" : "false");
                    break;
                case Kind.Number:
                    sb.Append(NumVal.ToString("G", CultureInfo.InvariantCulture));
                    break;
                case Kind.String:
                    WriteEscapedString(sb, StrVal ?? string.Empty);
                    break;
                case Kind.Array:
                    sb.Append('[');
                    for (int i = 0; i < ArrVal.Count; i++)
                    {
                        if (i > 0) sb.Append(',');
                        ArrVal[i].WriteTo(sb);
                    }
                    sb.Append(']');
                    break;
                case Kind.Object:
                    sb.Append('{');
                    bool first = true;
                    foreach (var kv in ObjVal)
                    {
                        if (!first) sb.Append(',');
                        first = false;
                        WriteEscapedString(sb, kv.Key);
                        sb.Append(':');
                        (kv.Value ?? Null()).WriteTo(sb);
                    }
                    sb.Append('}');
                    break;
            }
        }

        private static void WriteEscapedString(StringBuilder sb, string s)
        {
            sb.Append('"');
            for (int i = 0; i < s.Length; i++)
            {
                char c = s[i];
                switch (c)
                {
                    case '"': sb.Append("\\\""); break;
                    case '\\': sb.Append("\\\\"); break;
                    case '\b': sb.Append("\\b"); break;
                    case '\f': sb.Append("\\f"); break;
                    case '\n': sb.Append("\\n"); break;
                    case '\r': sb.Append("\\r"); break;
                    case '\t': sb.Append("\\t"); break;
                    default:
                        if (c < 0x20)
                            sb.AppendFormat(CultureInfo.InvariantCulture, "\\u{0:x4}", (int)c);
                        else
                            sb.Append(c);
                        break;
                }
            }
            sb.Append('"');
        }

        public static JsonVal Parse(string json)
        {
            if (string.IsNullOrEmpty(json)) return Null();
            int pos = 0;
            try
            {
                return ParseValue(json, ref pos);
            }
            catch
            {
                return Null();
            }
        }

        private static void SkipWs(string s, ref int p)
        {
            while (p < s.Length && char.IsWhiteSpace(s[p])) p++;
        }

        private static JsonVal ParseValue(string s, ref int p)
        {
            SkipWs(s, ref p);
            if (p >= s.Length) return Null();
            char c = s[p];
            if (c == '{') return ParseObject(s, ref p);
            if (c == '[') return ParseArray(s, ref p);
            if (c == '"') return FromStr(ParseStringRaw(s, ref p));
            if (c == 't' && p + 4 <= s.Length && s.Substring(p, 4) == "true") { p += 4; return FromBool(true); }
            if (c == 'f' && p + 5 <= s.Length && s.Substring(p, 5) == "false") { p += 5; return FromBool(false); }
            if (c == 'n' && p + 4 <= s.Length && s.Substring(p, 4) == "null") { p += 4; return Null(); }
            return ParseNumber(s, ref p);
        }

        private static JsonVal ParseObject(string s, ref int p)
        {
            var obj = NewObject();
            p++; // '{'
            SkipWs(s, ref p);
            if (p < s.Length && s[p] == '}') { p++; return obj; }
            while (p < s.Length)
            {
                SkipWs(s, ref p);
                if (p >= s.Length || s[p] != '"') break;
                string key = ParseStringRaw(s, ref p);
                SkipWs(s, ref p);
                if (p < s.Length && s[p] == ':') p++;
                obj[key] = ParseValue(s, ref p);
                SkipWs(s, ref p);
                if (p < s.Length && s[p] == ',') { p++; continue; }
                if (p < s.Length && s[p] == '}') { p++; break; }
                break;
            }
            return obj;
        }

        private static JsonVal ParseArray(string s, ref int p)
        {
            var arr = NewArray();
            p++; // '['
            SkipWs(s, ref p);
            if (p < s.Length && s[p] == ']') { p++; return arr; }
            while (p < s.Length)
            {
                arr.Add(ParseValue(s, ref p));
                SkipWs(s, ref p);
                if (p < s.Length && s[p] == ',') { p++; continue; }
                if (p < s.Length && s[p] == ']') { p++; break; }
                break;
            }
            return arr;
        }

        private static string ParseStringRaw(string s, ref int p)
        {
            var sb = new StringBuilder();
            p++; // '"'
            while (p < s.Length)
            {
                char c = s[p++];
                if (c == '"') break;
                if (c == '\\' && p < s.Length)
                {
                    char esc = s[p++];
                    switch (esc)
                    {
                        case '"': sb.Append('"'); break;
                        case '\\': sb.Append('\\'); break;
                        case '/': sb.Append('/'); break;
                        case 'b': sb.Append('\b'); break;
                        case 'f': sb.Append('\f'); break;
                        case 'n': sb.Append('\n'); break;
                        case 'r': sb.Append('\r'); break;
                        case 't': sb.Append('\t'); break;
                        case 'u':
                            if (p + 4 <= s.Length &&
                                int.TryParse(s.Substring(p, 4), NumberStyles.HexNumber, null, out int cp))
                            {
                                sb.Append((char)cp);
                                p += 4;
                            }
                            break;
                        default: sb.Append(esc); break;
                    }
                }
                else
                {
                    sb.Append(c);
                }
            }
            return sb.ToString();
        }

        private static JsonVal ParseNumber(string s, ref int p)
        {
            int start = p;
            if (p < s.Length && (s[p] == '-' || s[p] == '+')) p++;
            while (p < s.Length && (char.IsDigit(s[p]) || s[p] == '.' || s[p] == 'e' || s[p] == 'E' || s[p] == '+' || s[p] == '-'))
                p++;
            string tok = s.Substring(start, p - start);
            if (double.TryParse(tok, NumberStyles.Float, CultureInfo.InvariantCulture, out double d))
                return FromNum(d);
            return FromNum(0.0);
        }
    }
}
