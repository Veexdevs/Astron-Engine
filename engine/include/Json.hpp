#pragma once
#include <map>
#include <string>
#include <vector>

namespace astron {

class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    using Array  = std::vector<Json>;
    using Object = std::map<std::string, Json>;

    Json() : m_type(Type::Null) {}
    Json(std::nullptr_t) : m_type(Type::Null) {}
    Json(bool b) : m_type(Type::Bool), m_bool(b) {}
    Json(int n) : m_type(Type::Number), m_num(static_cast<double>(n)) {}
    Json(double d) : m_type(Type::Number), m_num(d) {}
    Json(const char* s) : m_type(Type::String), m_str(s ? s : "") {}
    Json(std::string s) : m_type(Type::String), m_str(std::move(s)) {}
    Json(Array a) : m_type(Type::Array), m_arr(std::move(a)) {}
    Json(Object o) : m_type(Type::Object), m_obj(std::move(o)) {}

    static Json array() { return Json(Array{}); }
    static Json object() { return Json(Object{}); }

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Bool; }
    bool isNumber() const { return m_type == Type::Number; }
    bool isString() const { return m_type == Type::String; }
    bool isArray() const { return m_type == Type::Array; }
    bool isObject() const { return m_type == Type::Object; }

    bool boolean(bool def = false) const { return isBool() ? m_bool : def; }
    double num(double def = 0.0) const { return isNumber() ? m_num : def; }
    int integer(int def = 0) const { return isNumber() ? static_cast<int>(m_num) : def; }
    const std::string& str() const {
        static const std::string kEmpty;
        return isString() ? m_str : kEmpty;
    }
    std::string strOr(const std::string& def) const { return isString() ? m_str : def; }

    const Array& asArray() const { return m_arr; }
    Array& asArray() {
        if (m_type != Type::Array) { m_type = Type::Array; m_arr.clear(); }
        return m_arr;
    }
    const Object& asObject() const { return m_obj; }
    Object& asObject() {
        if (m_type != Type::Object) { m_type = Type::Object; m_obj.clear(); }
        return m_obj;
    }

    size_t size() const {
        if (isArray()) return m_arr.size();
        if (isObject()) return m_obj.size();
        return 0;
    }

    void push_back(Json v) {
        asArray().push_back(std::move(v));
    }

    bool has(const std::string& key) const {
        return isObject() && m_obj.find(key) != m_obj.end();
    }

    Json& operator[](const std::string& key) {
        return asObject()[key];
    }

    const Json& operator[](const std::string& key) const {
        static const Json kNull;
        if (!isObject()) return kNull;
        auto it = m_obj.find(key);
        return it != m_obj.end() ? it->second : kNull;
    }

    const Json& at(size_t idx) const {
        static const Json kNull;
        if (!isArray() || idx >= m_arr.size()) return kNull;
        return m_arr[idx];
    }

    std::string dump(int indent = -1) const;
    static Json parse(const std::string& text, std::string* outError = nullptr);

private:
    void dumpImpl(std::string& out, int indent, int depth) const;

    Type m_type = Type::Null;
    bool m_bool = false;
    double m_num = 0.0;
    std::string m_str;
    Array m_arr;
    Object m_obj;
};

} // namespace astron
