#pragma once
#include <string>
#include <optional>

namespace Lei
{
    enum class TokenType {
        // Primitive Types
        INTEGER,
        CHARACTER,
        STRING,
        BOOLEAN,


        // Keywords
        IF,
        ELSE,
        WHILE,
        RETURN,
        FUNCTION, // Like fn void main() {}
        VOID,

        // Operators
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        LESS_THAN,
        GREATER_THAN,
        EQUAL,
        ASSIGNMENT,

        // Punctuation
        LEFT_BRACE,
        RIGHT_BRACE,
        LEFT_PARENTHESIS,
        RIGHT_PARENTHESIS,
        SEMICOLON,
        COMMA,

        // Special Tokens
        IDENTIFIER,
        INTEGER_LITERAL,    // Numeric literal like 124
        CHAR_LITERAL,       // Character literal like 'a'
        STRING_LITERAL,     // String literal like "hello"
        INTERPOLATED_STRING_LITERAL, // String with interpolation like "Hello, ${name}"
        BOOL_LITERAL,       // Boolean literal (0/1 or true/false)
        END_OF_FILE
    };


    struct Token {
        TokenType type;
        std::optional<std::string> value;
        int line;
        int column;
    };
} // namespace Lei

