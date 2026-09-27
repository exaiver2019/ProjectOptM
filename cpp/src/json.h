// Minimal JSON reader - enough for Project OptM's settings.json.
#pragma once
#include <map>
#include <string>
#include <vector>

struct Json {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::vector<Json> arr;
    std::map<std::string, Json> obj;

    const Json& operator[](const std::string& key) const;   // missing key -> Null
    std::string AsString(const std::string& def = "") const { return type == String ? str : def; }
    bool AsBool(bool def) const { return type == Bool ? b : def; }

    static Json Parse(const std::string& text);
    std::string Dump(int indent = 0) const;
    static Json Str(const std::string& s) { Json j; j.type = String; j.str = s; return j; }
    static Json Obj() { Json j; j.type = Object; return j; }
    static Json Arr() { Json j; j.type = Array; return j; }
    static Json Boolean(bool v) { Json j; j.type = Bool; j.b = v; return j; }
    static Json Num(double v) { Json j; j.type = Number; j.num = v; return j; }
    static Json StrList(const std::vector<std::string>& v) { Json j = Arr(); for (auto& s : v) j.arr.push_back(Str(s)); return j; }
    std::vector<std::string> AsStrings() const {   // array of strings, or a single string
        std::vector<std::string> out;
        if (type == String) out.push_back(str);
        for (auto& e : arr) if (e.type == String) out.push_back(e.str);
        return out;
    }
};
