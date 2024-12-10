#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <stdexcept>
#include <unordered_map>

// Common types and enums used across the project
// In lexer.h
enum TokenType {
    // Keywords
    FN,
    INT,
    FLOAT_TYPE,
    BOOL_TYPE,
    STRING_TYPE,
    VAR,
    RETURN,
    IF,
    ELSE,
    WHILE,
    FOR,
    PRINT,

    // Literals
    IDENTIFIER,
    NUMBER,
    FLOAT_LITERAL,
    BOOL_LITERAL,
    STRING,

    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    EQUALS,
    PLUS_EQUALS,
    MINUS_EQUALS,
    STAR_EQUALS,
    SLASH_EQUALS,
    EQUALS_EQUALS,
    NOT_EQUALS,
    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL,
    AND,
    OR,
    NOT,

    // Delimiters
    STRING_START,     // For " that starts string with interpolation
    STRING_END,       // For " that ends string
    STRING_CONTENT,   // For regular string content
    INTERPOLATION_START,  // For ${
    INTERPOLATION_END,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    COMMA,
    COLON,

    // Special
    END
};

enum class Type {
    Int,
    Float,
    String,
    Void,
    Bool,
    Unknown
};

// Variant type for return values
using ReturnValue = std::variant<int, float, std::string>;


#endif // COMMON_H