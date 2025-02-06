#include "evm/SolidityParser.h"
#include <sstream>
#include <regex>
#include <cctype>

namespace evm {

namespace {
    const std::vector<std::string> KEYWORDS = {
        "contract", "function", "public", "private", "internal", "external",
        "pure", "view", "payable", "returns", "memory", "storage", "calldata",
        "uint", "int", "address", "bool", "string", "bytes", "mapping",
        "struct", "enum", "if", "else", "while", "for", "do", "break",
        "continue", "return"
    };

    bool is_keyword(const std::string& str) {
        return std::find(KEYWORDS.begin(), KEYWORDS.end(), str) != KEYWORDS.end();
    }

    bool is_number_literal(const std::string& str) {
        return std::regex_match(str, std::regex("^[0-9]+$"));
    }
}

std::vector<SolidityParser::Token> SolidityParser::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    std::istringstream stream(source);
    std::string current_token;
    size_t line = 1, column = 1;

    auto add_token = [&](Token::Type type, const std::string& value) {
        tokens.push_back({type, value, line, column});
        column += value.length();
    };

    char c;
    while (stream.get(c)) {
        if (std::isspace(c)) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            if (!current_token.empty()) {
                if (is_keyword(current_token)) {
                    add_token(Token::Type::KEYWORD, current_token);
                } else if (is_number_literal(current_token)) {
                    add_token(Token::Type::NUMBER, current_token);
                } else {
                    add_token(Token::Type::IDENTIFIER, current_token);
                }
                current_token.clear();
            }
            continue;
        }

        if (std::isalnum(c) || c == '_') {
            current_token += c;
            continue;
        }

        // Handle operators and punctuation
        if (!current_token.empty()) {
            if (is_keyword(current_token)) {
                add_token(Token::Type::KEYWORD, current_token);
            } else if (is_number_literal(current_token)) {
                add_token(Token::Type::NUMBER, current_token);
            } else {
                add_token(Token::Type::IDENTIFIER, current_token);
            }
            current_token.clear();
        }

        std::string op;
        op += c;

        // Multi-character operators
        if (c == '/' && stream.peek() == '/') {
            // Single-line comment
            std::string comment = "//";
            while (stream.get(c) && c != '\n') {
                comment += c;
            }
            add_token(Token::Type::COMMENT, comment);
            line++;
            column = 1;
            continue;
        }

        if (c == '/' && stream.peek() == '*') {
            // Multi-line comment
            std::string comment = "/*";
            stream.get(c); // consume *
            while (stream.get(c)) {
                comment += c;
                if (c == '*' && stream.peek() == '/') {
                    stream.get(c); // consume /
                    comment += c;
                    break;
                }
                if (c == '\n') {
                    line++;
                    column = 1;
                }
            }
            add_token(Token::Type::COMMENT, comment);
            continue;
        }

        // String literals
        if (c == '"') {
            std::string str = "\"";
            while (stream.get(c) && c != '"') {
                if (c == '\\') {
                    str += c;
                    if (stream.get(c)) {
                        str += c;
                    }
                } else {
                    str += c;
                }
            }
            str += '"';
            add_token(Token::Type::STRING, str);
            continue;
        }

        // Single-character tokens
        if (std::string("(){}[];,").find(c) != std::string::npos) {
            add_token(Token::Type::PUNCTUATION, op);
        } else {
            add_token(Token::Type::OPERATOR, op);
        }
    }

    // Add any remaining token
    if (!current_token.empty()) {
        if (is_keyword(current_token)) {
            add_token(Token::Type::KEYWORD, current_token);
        } else if (is_number_literal(current_token)) {
            add_token(Token::Type::NUMBER, current_token);
        } else {
            add_token(Token::Type::IDENTIFIER, current_token);
        }
    }

    add_token(Token::Type::END, "");
    return tokens;
}

SolidityParser::ParseResult SolidityParser::parse(const std::string& source_code) {
    ParseResult result;
    result.success = false;

    try {
        auto tokens = tokenize(source_code);
        // TODO: Implement full parsing
        result.success = true;
    } catch (const std::exception& e) {
        result.errors.push_back(e.what());
    }

    return result;
}

TypeInfo SolidityParser::resolve_type(const std::string& type_string) {
    TypeInfo info;
    
    // Handle arrays
    auto pos = type_string.find('[');
    if (pos != std::string::npos) {
        info.is_array = true;
        std::string base_type = type_string.substr(0, pos);
        std::string array_spec = type_string.substr(pos);
        
        // Parse array size if specified
        if (array_spec != "[]") {
            try {
                info.array_size = std::stoul(array_spec.substr(1, array_spec.length() - 2));
            } catch (...) {
                add_error("Invalid array size specification", 0, 0);
            }
        }
        
        // Recursively resolve base type
        auto base_info = resolve_type(base_type);
        info.base_type = base_info.base_type;
        info.bits = base_info.bits;
        return info;
    }
    
    // Handle mappings
    if (type_string.substr(0, 7) == "mapping") {
        info.is_mapping = true;
        // TODO: Parse mapping key and value types
        return info;
    }
    
    // Handle basic types
    if (type_string.substr(0, 4) == "uint") {
        info.base_type = DataType::UINT;
        if (type_string.length() > 4) {
            info.bits = std::stoul(type_string.substr(4));
        }
    } else if (type_string.substr(0, 3) == "int") {
        info.base_type = DataType::INT;
        if (type_string.length() > 3) {
            info.bits = std::stoul(type_string.substr(3));
        }
    } else if (type_string == "address") {
        info.base_type = DataType::ADDRESS;
    } else if (type_string == "bool") {
        info.base_type = DataType::BOOL;
    } else if (type_string == "string") {
        info.base_type = DataType::STRING;
    } else if (type_string.substr(0, 5) == "bytes") {
        info.base_type = DataType::BYTES;
        if (type_string.length() > 5) {
            info.bits = std::stoul(type_string.substr(5)) * 8;
        }
    }
    
    return info;
}

void SolidityParser::add_error(const std::string& message, size_t line, size_t column) {
    std::stringstream ss;
    ss << "Error at line " << line << ", column " << column << ": " << message;
    errors_.push_back(ss.str());
}

void SolidityParser::add_warning(const std::string& message, size_t line, size_t column) {
    std::stringstream ss;
    ss << "Warning at line " << line << ", column " << column << ": " << message;
    warnings_.push_back(ss.str());
}

} // namespace evm 