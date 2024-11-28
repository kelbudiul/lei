#pragma once
#include <string>

enum class TokenType {
    IDENTIFIER,
    KEYWORD,
    KEYWORD_INT,
    KEYWORD_BOOL,
    KEYWORD_STRING,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    LITERAL_INTEGER,
    LITERAL_STRING,
    OPERATOR,
    OPERATOR_ASSIGN,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MULT,
    OPERATOR_DIV,
    OPERATOR_MOD,
    PUNCTUATION,
    COMMENT,
    SEMICOLON,
    UNKNOWN,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPERATOR_BLSHIFT,
    OPERATOR_BRSHIFT,
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};
