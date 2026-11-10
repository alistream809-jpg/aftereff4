#pragma once
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
namespace at {
// Small self-contained JSON value (no external dependency). Numbers are doubles.
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;

    Json() = default;
    Json(std::nullptr_t) {}
    Json(bool v) : type_(Type::Bool), b_(v) {}
    Json(int v) : type_(Type::Number), n_(v) {}
    Json(double v) : type_(Type::Number), n_(v) {}
    Json(const char* v) : type_(Type::String), s_(v) {}
    Json(std::string v) : type_(Type::String), s_(std::move(v)) {}
    static Json array() { Json j; j.type_ = Type::Array; return j; }
    static Json object() { Json j; j.type_ = Type::Object; return j; }

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }

    bool asBool() const { need(Type::Bool); return b_; }
    double asNumber() const { need(Type::Number); return n_; }
    const std::string& asString() const { need(Type::String); return s_; }
    const Array& asArray() const { need(Type::Array); return a_; }
    const Object& asObject() const { need(Type::Object); return o_; }

    // Building
    Json& operator[](const std::string& key) {
        if (type_ == Type::Null) type_ = Type::Object;
        need(Type::Object);
        return o_[key];
    }
    void push(Json v) {
        if (type_ == Type::Null) type_ = Type::Array;
        need(Type::Array);
        a_.push_back(std::move(v));
    }

    // Reading with defaults (never throw)
    bool has(const std::string& key) const { return type_ == Type::Object && o_.count(key) > 0; }
    const Json& get(const std::string& key) const;   // returns a Null value if missing
    double getNumber(const std::string& key, double def) const;
    std::string getString(const std::string& key, const std::string& def) const;
    bool getBool(const std::string& key, bool def) const;

    std::string dump(int indent = 2) const;
    static Json parse(const std::string& text);   // throws std::runtime_error

private:
    void need(Type t) const { if (type_ != t) throw std::runtime_error("JSON type mismatch"); }
    void dumpTo(std::string& out, int indent, int level) const;
    Type type_ = Type::Null;
    bool b_ = false;
    double n_ = 0;
    std::string s_;
    Array a_;
    Object o_;
};
}
