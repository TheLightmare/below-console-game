#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <cctype>

namespace json {

// Forward declaration
class Value;

using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;
using Null = std::nullptr_t;

// JSON Value - can hold any JSON type
class Value {
public:
    using Data = std::variant<Null, bool, double, std::string, Array, Object>;
    
private:
    Data data;
    
public:
    Value() : data(nullptr) {}
    Value(std::nullptr_t) : data(nullptr) {}
    Value(bool b) : data(b) {}
    Value(int i) : data(static_cast<double>(i)) {}
    Value(double d) : data(d) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(const std::string& s) : data(s) {}
    Value(std::string&& s) : data(std::move(s)) {}
    Value(const Array& arr) : data(arr) {}
    Value(Array&& arr) : data(std::move(arr)) {}
    Value(const Object& obj) : data(obj) {}
    Value(Object&& obj) : data(std::move(obj)) {}
    
    // Type checks
    bool is_null() const { return std::holds_alternative<Null>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    bool is_number() const { return std::holds_alternative<double>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_array() const { return std::holds_alternative<Array>(data); }
    bool is_object() const { return std::holds_alternative<Object>(data); }
    
    // Getters
    bool as_bool() const { return std::get<bool>(data); }
    double as_number() const { return std::get<double>(data); }
    int as_int() const { return static_cast<int>(std::get<double>(data)); }
    const std::string& as_string() const { return std::get<std::string>(data); }
    const Array& as_array() const { return std::get<Array>(data); }
    const Object& as_object() const { return std::get<Object>(data); }
    Array& as_array() { return std::get<Array>(data); }
    Object& as_object() { return std::get<Object>(data); }
    
    // Safe getters with defaults
    bool get_bool(bool default_val = false) const {
        return is_bool() ? as_bool() : default_val;
    }
    double get_number(double default_val = 0.0) const {
        return is_number() ? as_number() : default_val;
    }
    int get_int(int default_val = 0) const {
        return is_number() ? as_int() : default_val;
    }
    std::string get_string(const std::string& default_val = "") const {
        return is_string() ? as_string() : default_val;
    }
    
    // Array access
    const Value& operator[](size_t index) const {
        static Value null_value;
        if (!is_array() || index >= as_array().size()) return null_value;
        return as_array()[index];
    }
    
    // Object access
    const Value& operator[](const std::string& key) const {
        static Value null_value;
        if (!is_object()) return null_value;
        auto it = as_object().find(key);
        return it != as_object().end() ? it->second : null_value;
    }
    
    // Check if key exists
    bool has(const std::string& key) const {
        return is_object() && as_object().find(key) != as_object().end();
    }
    
    // Size
    size_t size() const {
        if (is_array()) return as_array().size();
        if (is_object()) return as_object().size();
        return 0;
    }
};

// JSON Parser
class Parser {
private:
    std::string text;
    size_t pos = 0;
    
    void skip_whitespace() {
        while (pos < text.size() && std::isspace(text[pos])) {
            pos++;
        }
        // Skip comments (non-standard but useful)
        if (pos < text.size() - 1 && text[pos] == '/' && text[pos + 1] == '/') {
            while (pos < text.size() && text[pos] != '\n') pos++;
            skip_whitespace();
        }
    }
    
    char peek() {
        skip_whitespace();
        return pos < text.size() ? text[pos] : '\0';
    }
    
    char get() {
        skip_whitespace();
        return pos < text.size() ? text[pos++] : '\0';
    }
    
    bool match(char expected) {
        if (peek() == expected) {
            pos++;
            return true;
        }
        return false;
    }
    
    void expect(char expected) {
        char c = get();
        if (c != expected) {
            throw std::runtime_error(std::string("Expected '") + expected + "' but got '" + c + "'");
        }
    }
    
    std::string parse_string() {
        expect('"');
        std::string result;
        while (pos < text.size() && text[pos] != '"') {
            if (text[pos] == '\\' && pos + 1 < text.size()) {
                pos++;
                switch (text[pos]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    case '/': result += '/'; break;
                    default: result += text[pos]; break;
                }
            } else {
                result += text[pos];
            }
            pos++;
        }
        expect('"');
        return result;
    }
    
    Value parse_number() {
        size_t start = pos;
        if (text[pos] == '-') pos++;
        while (pos < text.size() && (std::isdigit(text[pos]) || text[pos] == '.' || 
               text[pos] == 'e' || text[pos] == 'E' || text[pos] == '+' || text[pos] == '-')) {
            pos++;
        }
        return Value(std::stod(text.substr(start, pos - start)));
    }
    
    Value parse_array() {
        expect('[');
        Array arr;
        if (peek() != ']') {
            do {
                arr.push_back(parse_value());
            } while (match(','));
        }
        expect(']');
        return Value(std::move(arr));
    }
    
    Value parse_object() {
        expect('{');
        Object obj;
        if (peek() != '}') {
            do {
                std::string key = parse_string();
                expect(':');
                obj[key] = parse_value();
            } while (match(','));
        }
        expect('}');
        return Value(std::move(obj));
    }
    
    Value parse_value() {
        skip_whitespace();
        char c = peek();
        
        if (c == '"') return Value(parse_string());
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || std::isdigit(c)) return parse_number();
        
        // Keywords
        if (text.compare(pos, 4, "true") == 0) { pos += 4; return Value(true); }
        if (text.compare(pos, 5, "false") == 0) { pos += 5; return Value(false); }
        if (text.compare(pos, 4, "null") == 0) { pos += 4; return Value(nullptr); }
        
        throw std::runtime_error(std::string("Unexpected character: ") + c);
    }
    
public:
    Value parse(const std::string& json_text) {
        text = json_text;
        pos = 0;
        return parse_value();
    }
};

// Utility functions
inline Value parse(const std::string& json_text) {
    Parser parser;
    return parser.parse(json_text);
}

inline Value parse_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

inline bool try_parse_file(const std::string& filename, Value& out) {
    try {
        out = parse_file(filename);
        return true;
    } catch (...) {
        return false;
    }
}

inline bool try_parse(const std::string& json_text, Value& out) {
    try {
        out = parse(json_text);
        return true;
    } catch (...) {
        return false;
    }
}

// Serialize a JSON value to string
inline std::string stringify(const Value& val, int indent = 0, int current_indent = 0) {
    std::string result;
    std::string ind(current_indent, ' ');
    std::string ind_next(current_indent + indent, ' ');
    
    if (val.is_null()) {
        return "null";
    }
    if (val.is_bool()) {
        return val.as_bool() ? "true" : "false";
    }
    if (val.is_number()) {
        double d = val.as_number();
        if (d == static_cast<int>(d)) {
            return std::to_string(static_cast<int>(d));
        }
        // Use ostringstream for consistent formatting
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }
    if (val.is_string()) {
        result = "\"";
        for (char c : val.as_string()) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        result += "\"";
        return result;
    }
    if (val.is_array()) {
        const Array& arr = val.as_array();
        if (arr.empty()) return "[]";
        result = "[";
        if (indent > 0) result += "\n";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (indent > 0) result += ind_next;
            result += stringify(arr[i], indent, current_indent + indent);
            if (i < arr.size() - 1) result += ",";
            if (indent > 0) result += "\n";
        }
        if (indent > 0) result += ind;
        result += "]";
        return result;
    }
    if (val.is_object()) {
        const Object& obj = val.as_object();
        if (obj.empty()) return "{}";
        result = "{";
        if (indent > 0) result += "\n";
        size_t i = 0;
        for (const auto& [key, value] : obj) {
            if (indent > 0) result += ind_next;
            result += "\"" + key + "\": " + stringify(value, indent, current_indent + indent);
            if (i < obj.size() - 1) result += ",";
            if (indent > 0) result += "\n";
            ++i;
        }
        if (indent > 0) result += ind;
        result += "}";
        return result;
    }
    return "null";
}

// Write JSON to file
inline bool write_file(const std::string& filename, const Value& val, int indent = 2) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    file << stringify(val, indent);
    return true;
}

} // namespace json
