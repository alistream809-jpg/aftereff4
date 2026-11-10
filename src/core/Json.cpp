#include "core/Json.h"
#include <charconv>
#include <cmath>
#include <cstdio>
namespace at {

const Json& Json::get(const std::string& key) const {
    static const Json nullJson;
    if (type_ != Type::Object) return nullJson;
    auto it = o_.find(key);
    return it == o_.end() ? nullJson : it->second;
}
double Json::getNumber(const std::string& k, double d) const { const Json& j = get(k); return j.isNumber() ? j.n_ : d; }
std::string Json::getString(const std::string& k, const std::string& d) const { const Json& j = get(k); return j.isString() ? j.s_ : d; }
bool Json::getBool(const std::string& k, bool d) const { const Json& j = get(k); return j.type_ == Type::Bool ? j.b_ : d; }

// ---------------- writer ----------------
static void escapeTo(std::string& out, const std::string& s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); out += b; }
                else out += static_cast<char>(c);
        }
    }
    out += '"';
}
static void numberTo(std::string& out, double v) {
    if (!std::isfinite(v)) { out += "null"; return; }
    char buf[64];
    if (v == std::floor(v) && std::fabs(v) < 1e15) {
        auto r = std::to_chars(buf, buf + sizeof buf, static_cast<long long>(v));
        out.append(buf, r.ptr);
    } else {
        auto r = std::to_chars(buf, buf + sizeof buf, v);   // shortest round-trip, locale independent
        out.append(buf, r.ptr);
    }
}
void Json::dumpTo(std::string& out, int indent, int level) const {
    auto nl = [&](int lv) { if (indent > 0) { out += '\n'; out.append(static_cast<size_t>(lv * indent), ' '); } };
    switch (type_) {
        case Type::Null: out += "null"; break;
        case Type::Bool: out += b_ ? "true" : "false"; break;
        case Type::Number: numberTo(out, n_); break;
        case Type::String: escapeTo(out, s_); break;
        case Type::Array:
            if (a_.empty()) { out += "[]"; break; }
            out += '[';
            for (size_t i = 0; i < a_.size(); ++i) {
                if (i) out += ',';
                nl(level + 1);
                a_[i].dumpTo(out, indent, level + 1);
            }
            nl(level); out += ']';
            break;
        case Type::Object:
            if (o_.empty()) { out += "{}"; break; }
            out += '{';
            { bool first = true;
              for (const auto& kv : o_) {
                  if (!first) out += ',';
                  first = false;
                  nl(level + 1);
                  escapeTo(out, kv.first);
                  out += indent > 0 ? ": " : ":";
                  kv.second.dumpTo(out, indent, level + 1);
              } }
            nl(level); out += '}';
            break;
    }
}
std::string Json::dump(int indent) const { std::string s; dumpTo(s, indent, 0); if (indent > 0) s += '\n'; return s; }

// ---------------- parser ----------------
namespace {
struct Parser {
    const std::string& s;
    size_t i = 0;
    int depth = 0;
    explicit Parser(const std::string& str) : s(str) {}
    [[noreturn]] void fail(const std::string& m) {
        throw std::runtime_error("JSON parse error at offset " + std::to_string(i) + ": " + m);
    }
    void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i; }
    bool lit(const char* w) {
        size_t n = std::char_traits<char>::length(w);
        if (s.compare(i, n, w) == 0) { i += n; return true; }
        return false;
    }
    unsigned hex4() {
        if (i + 4 > s.size()) fail("bad \\u escape");
        unsigned v = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i++]; v <<= 4;
            if (c >= '0' && c <= '9') v |= c - '0';
            else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
            else fail("bad hex digit");
        }
        return v;
    }
    static void utf8(std::string& o, unsigned cp) {
        if (cp < 0x80) o += static_cast<char>(cp);
        else if (cp < 0x800) { o += static_cast<char>(0xC0 | (cp >> 6)); o += static_cast<char>(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { o += static_cast<char>(0xE0 | (cp >> 12)); o += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); o += static_cast<char>(0x80 | (cp & 0x3F)); }
        else { o += static_cast<char>(0xF0 | (cp >> 18)); o += static_cast<char>(0x80 | ((cp >> 12) & 0x3F)); o += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); o += static_cast<char>(0x80 | (cp & 0x3F)); }
    }
    std::string str() {
        ++i;  // opening quote
        std::string o;
        while (true) {
            if (i >= s.size()) fail("unterminated string");
            char c = s[i++];
            if (c == '"') break;
            if (c == '\\') {
                if (i >= s.size()) fail("bad escape");
                char e = s[i++];
                switch (e) {
                    case '"': o += '"'; break; case '\\': o += '\\'; break; case '/': o += '/'; break;
                    case 'b': o += '\b'; break; case 'f': o += '\f'; break; case 'n': o += '\n'; break;
                    case 'r': o += '\r'; break; case 't': o += '\t'; break;
                    case 'u': {
                        unsigned cp = hex4();
                        if (cp >= 0xD800 && cp < 0xDC00 && s.compare(i, 2, "\\u") == 0) {
                            i += 2; unsigned lo = hex4();
                            if (lo >= 0xDC00 && lo < 0xE000) cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            else fail("bad surrogate pair");
                        }
                        utf8(o, cp);
                        break;
                    }
                    default: fail("unknown escape");
                }
            } else o += c;
        }
        return o;
    }
    Json value() {
        ws();
        if (i >= s.size()) fail("unexpected end of input");
        if (++depth > 200) fail("nesting too deep");
        Json r;
        char c = s[i];
        if (c == '{') {
            ++i; r = Json::object(); ws();
            if (i < s.size() && s[i] == '}') ++i;
            else while (true) {
                ws();
                if (i >= s.size() || s[i] != '"') fail("expected string key");
                std::string k = str();
                ws();
                if (i >= s.size() || s[i] != ':') fail("expected ':'");
                ++i;
                r[k] = value();
                ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; break; }
                fail("expected ',' or '}'");
            }
        } else if (c == '[') {
            ++i; r = Json::array(); ws();
            if (i < s.size() && s[i] == ']') ++i;
            else while (true) {
                r.push(value());
                ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == ']') { ++i; break; }
                fail("expected ',' or ']'");
            }
        } else if (c == '"') r = Json(str());
        else if (lit("true")) r = Json(true);
        else if (lit("false")) r = Json(false);
        else if (lit("null")) r = Json();
        else {
            double v = 0;
            auto res = std::from_chars(s.data() + i, s.data() + s.size(), v);
            if (res.ec != std::errc()) fail("invalid value");
            i = static_cast<size_t>(res.ptr - s.data());
            r = Json(v);
        }
        --depth;
        return r;
    }
};
}
Json Json::parse(const std::string& text) {
    Parser p(text);
    Json v = p.value();
    p.ws();
    if (p.i != text.size()) p.fail("trailing characters");
    return v;
}
}
