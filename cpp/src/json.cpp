#include "json.h"
#include <cstdio>
#include <cstdlib>

namespace {
struct Parser {
    const std::string& s;
    size_t i = 0;
    explicit Parser(const std::string& text) : s(text) {}

    void Ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) i++; }

    static void Utf8(std::string& out, unsigned cp) {
        if (cp < 0x80) out += (char)cp;
        else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
        else { out += (char)(0xF0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
    }

    std::string Str() {
        std::string out;
        i++;  // opening quote
        while (i < s.size() && s[i] != '"') {
            char c = s[i++];
            if (c != '\\' || i >= s.size()) { out += c; continue; }
            char e = s[i++];
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    unsigned cp = (unsigned)strtoul(s.substr(i, 4).c_str(), nullptr, 16);
                    i += 4;
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                        unsigned lo = (unsigned)strtoul(s.substr(i + 2, 4).c_str(), nullptr, 16);
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        i += 6;
                    }
                    Utf8(out, cp);
                    break;
                }
                default: out += e;
            }
        }
        i++;  // closing quote
        return out;
    }

    Json Value() {
        Ws();
        Json v;
        if (i >= s.size()) return v;
        char c = s[i];
        if (c == '{') {
            v.type = Json::Object; i++;
            for (;;) {
                Ws();
                if (i < s.size() && s[i] == '}') { i++; break; }
                if (i >= s.size() || s[i] != '"') break;
                std::string k = Str();
                Ws(); if (i < s.size() && s[i] == ':') i++;
                v.obj[k] = Value();
                Ws();
                if (i < s.size() && s[i] == ',') { i++; continue; }
                if (i < s.size() && s[i] == '}') i++;
                break;
            }
        } else if (c == '[') {
            v.type = Json::Array; i++;
            for (;;) {
                Ws();
                if (i < s.size() && s[i] == ']') { i++; break; }
                v.arr.push_back(Value());
                Ws();
                if (i < s.size() && s[i] == ',') { i++; continue; }
                if (i < s.size() && s[i] == ']') i++;
                break;
            }
        } else if (c == '"') {
            v.type = Json::String; v.str = Str();
        } else if (s.compare(i, 4, "true") == 0)  { v.type = Json::Bool; v.b = true;  i += 4; }
        else if (s.compare(i, 5, "false") == 0)   { v.type = Json::Bool; v.b = false; i += 5; }
        else if (s.compare(i, 4, "null") == 0)    { i += 4; }
        else {
            char* end = nullptr;
            v.type = Json::Number; v.num = strtod(s.c_str() + i, &end);
            i = (end && end > s.c_str() + i) ? (size_t)(end - s.c_str()) : i + 1;
        }
        return v;
    }
};
}  // namespace

const Json& Json::operator[](const std::string& key) const {
    static const Json null;
    if (type != Object) return null;
    auto it = obj.find(key);
    return it == obj.end() ? null : it->second;
}

Json Json::Parse(const std::string& text) { Parser p(text); return p.Value(); }

namespace {
void Esc(std::string& o, const std::string& s) {
    o += '"';
    for (unsigned char ch : s) {
        switch (ch) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (ch < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", ch); o += b; }
                else o += (char)ch;
        }
    }
    o += '"';
}
}  // namespace

std::string Json::Dump(int indent) const {
    std::string o, pad(indent + 4, ' '), end(indent, ' ');
    switch (type) {
        case Null:   return "null";
        case Bool:   return b ? "true" : "false";
        case Number: { char buf[32]; snprintf(buf, sizeof(buf), "%.15g", num); return buf; }
        case String: Esc(o, str); return o;
        case Array:
            if (arr.empty()) return "[]";
            o = "[\r\n";
            for (size_t i = 0; i < arr.size(); i++) o += pad + arr[i].Dump(indent + 4) + (i + 1 < arr.size() ? ",\r\n" : "\r\n");
            return o + end + "]";
        case Object: {
            if (obj.empty()) return "{}";
            o = "{\r\n";
            size_t i = 0;
            for (auto& [k, v] : obj) { o += pad; Esc(o, k); o += ": " + v.Dump(indent + 4) + (++i < obj.size() ? ",\r\n" : "\r\n"); }
            return o + end + "}";
        }
    }
    return "null";
}
