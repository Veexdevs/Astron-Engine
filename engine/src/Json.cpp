#include "Json.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace astron {

namespace {

void escapeString(const std::string& s, std::string& out) {
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
    out.push_back('"');
}

void appendUtf8(uint32_t cp, std::string& out) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

class Parser {
public:
    explicit Parser(const std::string& s) : m_s(s) {}

    Json parseRoot(std::string& err) {
        skipWs();
        Json v = parseValue(err);
        if (!err.empty()) return Json();
        skipWs();
        return v;
    }

private:
    const std::string& m_s;
    size_t m_pos = 0;

    void skipWs() {
        while (m_pos < m_s.size()) {
            unsigned char c = static_cast<unsigned char>(m_s[m_pos]);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++m_pos;
            else break;
        }
    }

    bool match(const char* lit) {
        size_t n = 0;
        while (lit[n]) ++n;
        if (m_pos + n > m_s.size()) return false;
        if (m_s.compare(m_pos, n, lit) == 0) {
            m_pos += n;
            return true;
        }
        return false;
    }

    int hexVal(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    }

    bool parseHex4(uint32_t& out) {
        if (m_pos + 4 > m_s.size()) return false;
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            int h = hexVal(m_s[m_pos + i]);
            if (h < 0) return false;
            v = (v << 4) | static_cast<uint32_t>(h);
        }
        m_pos += 4;
        out = v;
        return true;
    }

    std::string parseString(std::string& err) {
        std::string out;
        if (m_pos >= m_s.size() || m_s[m_pos] != '"') {
            err = "Esperado '\"'";
            return out;
        }
        ++m_pos;
        while (m_pos < m_s.size()) {
            char c = m_s[m_pos++];
            if (c == '"') return out;
            if (c == '\\') {
                if (m_pos >= m_s.size()) { err = "Escape incompleto"; return out; }
                char esc = m_s[m_pos++];
                switch (esc) {
                    case '"':  out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/':  out.push_back('/'); break;
                    case 'b':  out.push_back('\b'); break;
                    case 'f':  out.push_back('\f'); break;
                    case 'n':  out.push_back('\n'); break;
                    case 'r':  out.push_back('\r'); break;
                    case 't':  out.push_back('\t'); break;
                    case 'u': {
                        uint32_t cp = 0;
                        if (!parseHex4(cp)) { err = "Escape \\u inválido"; return out; }
                        if (cp >= 0xD800 && cp <= 0xDBFF && m_pos + 2 <= m_s.size() &&
                            m_s[m_pos] == '\\' && m_s[m_pos + 1] == 'u') {
                            m_pos += 2;
                            uint32_t low = 0;
                            if (parseHex4(low) && low >= 0xDC00 && low <= 0xDFFF) {
                                cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                            }
                        }
                        appendUtf8(cp, out);
                        break;
                    }
                    default:
                        out.push_back(esc);
                        break;
                }
            } else {
                out.push_back(c);
            }
        }
        err = "String JSON não fechada";
        return out;
    }

    Json parseNumber(std::string& err) {
        const char* start = m_s.c_str() + m_pos;
        char* endPtr = nullptr;
        double val = std::strtod(start, &endPtr);
        if (endPtr == start) {
            err = "Número JSON inválido";
            return Json();
        }
        m_pos = static_cast<size_t>(endPtr - m_s.c_str());
        return Json(val);
    }

    Json parseArray(std::string& err) {
        ++m_pos; // '['
        Json::Array arr;
        skipWs();
        if (m_pos < m_s.size() && m_s[m_pos] == ']') {
            ++m_pos;
            return Json(std::move(arr));
        }
        while (m_pos < m_s.size()) {
            skipWs();
            Json item = parseValue(err);
            if (!err.empty()) return Json();
            arr.push_back(std::move(item));
            skipWs();
            if (m_pos < m_s.size() && m_s[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_pos < m_s.size() && m_s[m_pos] == ']') {
                ++m_pos;
                return Json(std::move(arr));
            }
            err = "Esperado ',' ou ']' no array JSON";
            return Json();
        }
        err = "Array JSON não fechado";
        return Json();
    }

    Json parseObject(std::string& err) {
        ++m_pos; // '{'
        Json::Object obj;
        skipWs();
        if (m_pos < m_s.size() && m_s[m_pos] == '}') {
            ++m_pos;
            return Json(std::move(obj));
        }
        while (m_pos < m_s.size()) {
            skipWs();
            std::string key = parseString(err);
            if (!err.empty()) return Json();
            skipWs();
            if (m_pos >= m_s.size() || m_s[m_pos] != ':') {
                err = "Esperado ':' no objeto JSON";
                return Json();
            }
            ++m_pos;
            skipWs();
            Json val = parseValue(err);
            if (!err.empty()) return Json();
            obj[std::move(key)] = std::move(val);
            skipWs();
            if (m_pos < m_s.size() && m_s[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_pos < m_s.size() && m_s[m_pos] == '}') {
                ++m_pos;
                return Json(std::move(obj));
            }
            err = "Esperado ',' ou '}' no objeto JSON";
            return Json();
        }
        err = "Objeto JSON não fechado";
        return Json();
    }

    Json parseValue(std::string& err) {
        skipWs();
        if (m_pos >= m_s.size()) {
            err = "Fim inesperado do JSON";
            return Json();
        }
        char c = m_s[m_pos];
        if (c == '"') return Json(parseString(err));
        if (c == '{') return parseObject(err);
        if (c == '[') return parseArray(err);
        if (match("true"))  return Json(true);
        if (match("false")) return Json(false);
        if (match("null"))  return Json(nullptr);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber(err);
        err = std::string("Caractere inesperado no JSON: ") + c;
        return Json();
    }
};

} // namespace

std::string Json::dump(int indent) const {
    std::string out;
    dumpImpl(out, indent, 0);
    return out;
}

void Json::dumpImpl(std::string& out, int indent, int depth) const {
    auto pad = [&](int d) {
        if (indent > 0) out.append(static_cast<size_t>(indent * d), ' ');
    };
    switch (m_type) {
        case Type::Null:
            out += "null";
            break;
        case Type::Bool:
            out += (m_bool ? "true" : "false");
            break;
        case Type::Number: {
            if (!std::isfinite(m_num)) {
                out += "0";
            } else if (std::floor(m_num) == m_num && std::fabs(m_num) < 1e15) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.0f", m_num);
                out += buf;
            } else {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.6g", m_num);
                out += buf;
            }
            break;
        }
        case Type::String:
            escapeString(m_str, out);
            break;
        case Type::Array: {
            if (m_arr.empty()) { out += "[]"; break; }
            out.push_back('[');
            if (indent > 0) out.push_back('\n');
            for (size_t i = 0; i < m_arr.size(); ++i) {
                pad(depth + 1);
                m_arr[i].dumpImpl(out, indent, depth + 1);
                if (i + 1 < m_arr.size()) out.push_back(',');
                if (indent > 0) out.push_back('\n');
            }
            pad(depth);
            out.push_back(']');
            break;
        }
        case Type::Object: {
            if (m_obj.empty()) { out += "{}"; break; }
            out.push_back('{');
            if (indent > 0) out.push_back('\n');
            size_t idx = 0;
            for (const auto& kv : m_obj) {
                pad(depth + 1);
                escapeString(kv.first, out);
                out += (indent > 0 ? ": " : ":");
                kv.second.dumpImpl(out, indent, depth + 1);
                if (++idx < m_obj.size()) out.push_back(',');
                if (indent > 0) out.push_back('\n');
            }
            pad(depth);
            out.push_back('}');
            break;
        }
    }
}

Json Json::parse(const std::string& text, std::string* outError) {
    std::string err;
    Parser p(text);
    Json res = p.parseRoot(err);
    if (outError) *outError = err;
    return res;
}

} // namespace astron
