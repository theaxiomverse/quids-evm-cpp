#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <map>

namespace evm {

// Forward declarations
class ASTNode;
class ContractDefinition;
class FunctionDefinition;

enum class DataType {
    UINT,
    INT,
    ADDRESS,
    BOOL,
    STRING,
    BYTES,
    ARRAY,
    MAPPING,
    STRUCT,
    ENUM
};

struct TypeInfo {
    DataType base_type;
    uint16_t bits{256};  // For uint/int
    bool is_array{false};
    bool is_mapping{false};
    std::optional<size_t> array_size;  // nullopt means dynamic
    std::shared_ptr<TypeInfo> value_type;  // For arrays and mappings
    std::shared_ptr<TypeInfo> key_type;    // For mappings
};

class SolidityParser {
public:
    struct ParseResult {
        std::shared_ptr<ASTNode> ast;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        bool success;
    };

    SolidityParser() = default;
    ~SolidityParser() = default;

    // Disable copy and move
    SolidityParser(const SolidityParser&) = delete;
    SolidityParser& operator=(const SolidityParser&) = delete;
    SolidityParser(SolidityParser&&) = delete;
    SolidityParser& operator=(SolidityParser&&) = delete;

    // Parse Solidity source code
    ParseResult parse(const std::string& source_code);
    
    // Parse a single contract
    std::shared_ptr<ContractDefinition> parse_contract(const std::string& contract_source);
    
    // Parse a single function
    std::shared_ptr<FunctionDefinition> parse_function(const std::string& function_source);
    
    // Type checking and validation
    bool validate_types(const ASTNode& node);
    TypeInfo resolve_type(const std::string& type_string);
    
    // Bytecode generation
    std::vector<uint8_t> generate_bytecode(const ASTNode& node);
    
private:
    // Lexical analysis
    struct Token {
        enum class Type {
            KEYWORD,
            IDENTIFIER,
            NUMBER,
            STRING,
            OPERATOR,
            PUNCTUATION,
            COMMENT,
            WHITESPACE,
            END
        };
        
        Type type;
        std::string value;
        size_t line;
        size_t column;
    };
    
    std::vector<Token> tokenize(const std::string& source);
    
    // Parsing helpers
    std::shared_ptr<ASTNode> parse_statement();
    std::shared_ptr<ASTNode> parse_expression();
    std::shared_ptr<ASTNode> parse_block();
    
    // Symbol table
    struct SymbolTable {
        std::map<std::string, TypeInfo> variables;
        std::map<std::string, std::shared_ptr<FunctionDefinition>> functions;
        std::shared_ptr<SymbolTable> parent;
    };
    
    std::shared_ptr<SymbolTable> current_scope_;
    
    // Error handling
    void add_error(const std::string& message, size_t line, size_t column);
    void add_warning(const std::string& message, size_t line, size_t column);
    
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    // Implementation details
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace evm 