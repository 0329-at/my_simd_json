#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <unordered_map>
#include <immintrin.h> // 用于 SIMD 指令

namespace json {

struct value;

using array = std::vector<value>;
using object = std::unordered_map<std::string, value>;

struct value {
    std::variant<std::nullptr_t, bool, double, std::string, array, object> data;

    template <typename T>
    T as() const {
        return std::get<T>(data);
    }

    const value& operator[](const std::string& key) const {
        return std::get<object>(data).at(key);
    }

    const value& operator[](size_t index) const {
        return std::get<array>(data).at(index);
    }

    std::string serialize() const;

private:
    std::string serialize_object(const object& obj) const;
    std::string serialize_array(const array& arr) const;
};

class parser {
public:
    value parse(std::string_view json_text);
    value parse_file(const std::string& file_path);

private:
    const char* skip_whitespace(const char* ptr);
    value parse_value(const char*& ptr);
    std::string parse_string(const char*& ptr);
    double parse_number(const char*& ptr);
    bool parse_bool(const char*& ptr);
    std::nullptr_t parse_null(const char*& ptr);
    array parse_array(const char*& ptr);
    object parse_object(const char*& ptr);
};

} // namespace json

