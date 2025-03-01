#include "json_parser.hpp"
#include <stdexcept>
#include <cctype>
#include <cstring>
#include <sstream>
#include <fstream> // 确保包含 <fstream> 头文件
#include <filesystem> // 用于路径处理

namespace json {

const char* parser::skip_whitespace(const char* ptr) {
    while (true) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
        __m256i spaces = _mm256_set1_epi8(' ');
        __m256i tabs = _mm256_set1_epi8('\t');
        __m256i newlines = _mm256_set1_epi8('\n');
        __m256i returns = _mm256_set1_epi8('\r');

        __m256i mask = _mm256_or_si256(
            _mm256_or_si256(_mm256_cmpeq_epi8(chunk, spaces), _mm256_cmpeq_epi8(chunk, tabs)),
            _mm256_or_si256(_mm256_cmpeq_epi8(chunk, newlines), _mm256_cmpeq_epi8(chunk, returns))
        );

        int bitmask = _mm256_movemask_epi8(mask);
        if (bitmask != 0xFFFFFFFF) {
            ptr += __builtin_ctz(~bitmask);
            break;
        }
        ptr += 32;
    }
    return ptr;
}

value parser::parse(std::string_view json_text) {
    const char* ptr = json_text.data();
    return parse_value(ptr);
}

value parser::parse_file(const std::string& file_path) {
    // 使用 std::filesystem 处理路径
    std::filesystem::path path(file_path);

    // 确保路径是绝对路径
    if (!path.is_absolute()) {
        path = std::filesystem::absolute(path);
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

value parser::parse_value(const char*& ptr) {
    ptr = skip_whitespace(ptr);
    switch (*ptr) {
        case '"': return value{parse_string(ptr)};
        case 't': return value{parse_bool(ptr)};
        case 'f': return value{parse_bool(ptr)};
        case 'n': return value{parse_null(ptr)};
        case '[': return value{parse_array(ptr)};
        case '{': return value{parse_object(ptr)};
        default: return value{parse_number(ptr)};
    }
}

std::string parser::parse_string(const char*& ptr) {
    ++ptr; // Skip opening quote
    const char* start = ptr;
    while (*ptr != '"') {
        if (*ptr == '\\') {
            ++ptr; // Skip escape character
        }
        ++ptr;
    }
    std::string result(start, ptr);
    ++ptr; // Skip closing quote
    return result;
}

double parser::parse_number(const char*& ptr) {
    char* end;
    double result = std::strtod(ptr, &end);
    if (ptr == end) {
        throw std::runtime_error("Invalid number");
    }
    ptr = end;
    return result;
}

bool parser::parse_bool(const char*& ptr) {
    if (std::strncmp(ptr, "true", 4) == 0) {
        ptr += 4;
        return true;
    } else if (std::strncmp(ptr, "false", 5) == 0) {
        ptr += 5;
        return false;
    }
    throw std::runtime_error("Invalid boolean value");
}

std::nullptr_t parser::parse_null(const char*& ptr) {
    if (std::strncmp(ptr, "null", 4) == 0) {
        ptr += 4;
        return nullptr;
    }
    throw std::runtime_error("Invalid null value");
}

array parser::parse_array(const char*& ptr) {
    ++ptr; // Skip opening bracket
    array result;
    while (*ptr != ']') {
        result.push_back(parse_value(ptr));
        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ++ptr;
        }
    }
    ++ptr; // Skip closing bracket
    return result;
}

object parser::parse_object(const char*& ptr) {
    ++ptr; // Skip opening brace
    object result;
    while (*ptr != '}') {
        ptr = skip_whitespace(ptr);
        std::string key = parse_string(ptr);
        ptr = skip_whitespace(ptr);
        if (*ptr != ':') {
            throw std::runtime_error("Expected ':' in object");
        }
        ++ptr; // Skip colon
        result[key] = parse_value(ptr);
        ptr = skip_whitespace(ptr);
        if (*ptr == ',') {
            ++ptr;
        } else if (*ptr != '}') {
            throw std::runtime_error("Expected ',' or '}' in object");
        }
    }
    ++ptr; // Skip closing brace
    return result;
}

std::string value::serialize() const {
    switch (data.index()) {
        case 0: return "null";
        case 1: return std::get<bool>(data) ? "true" : "false";
        case 2: return std::to_string(std::get<double>(data));
        case 3: return "\"" + std::get<std::string>(data) + "\"";
        case 4: return serialize_array(std::get<array>(data));
        case 5: return serialize_object(std::get<object>(data));
        default: throw std::runtime_error("Unknown type");
    }
}

std::string value::serialize_object(const object& obj) const {
    std::string result = "{";
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it != obj.begin()) {
            result += ",";
        }
        result += "\"" + it->first + "\":" + it->second.serialize();
    }
    result += "}";
    return result;
}

std::string value::serialize_array(const array& arr) const {
    std::string result = "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i != 0) {
            result += ",";
        }
        result += arr[i].serialize();
    }
    result += "]";
    return result;
}

} // namespace json