#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenType {
    // Keywords
    FN,             ///< Function declaration 'fn'
    INT,            ///< Integer type 'int'
    FLOAT_TYPE,     ///< Float type 'float'
    BOOL_TYPE,      ///< Boolean type 'bool'
    STRING_TYPE,    ///< String type 'str'
    VAR,            ///< Variable declaration 'var'
    RETURN,         ///< Return statement 'return'
    IF,             ///< If statement 'if'
    ELSE,           ///< Else statement 'else'
    WHILE,          ///< While loop 'while'
    
    // Literals
    IDENTIFIER,     ///< Variable/function names
    NUMBER,         ///< Integer literals
    FLOAT_LITERAL,  ///< Float literals
    STRING_LITERAL, ///< String literals
    BOOL_LITERAL,   ///< Boolean literals (true/false)
    
    // Operators
    PLUS,           ///< Addition '+'
    MINUS,          ///< Subtraction '-'
    STAR,           ///< Multiplication '*'
    SLASH,          ///< Division '/'
    PLUS_EQUALS,    ///< Add and assign '+='
    MINUS_EQUALS,   ///< Subtract and assign '-='
    STAR_EQUALS,    ///< Multiply and assign '*='
    SLASH_EQUALS,   ///< Divide and assign '/='
    EQUALS,         ///< Assignment '='
    EQUALS_EQUALS,  ///< Equality comparison '=='
    NOT_EQUALS,     ///< Inequality comparison '!='
    LESS,           ///< Less than '<'
    LESS_EQUAL,     ///< Less than or equal '<='
    GREATER,        ///< Greater than '>'
    GREATER_EQUAL,  ///< Greater than or equal '>='
    AND,            ///< Logical AND '&&'
    OR,             ///< Logical OR '||'
    NOT,            ///< Logical NOT '!'
    
    // Delimiters
    LPAREN,         ///< Left parenthesis '('
    RPAREN,         ///< Right parenthesis ')'
    LBRACE,         ///< Left brace '{'
    RBRACE,         ///< Right brace '}'
    LBRACKET,       ///< Left bracket '['
    RBRACKET,       ///< Right bracket ']'
    SEMICOLON,      ///< Semicolon ';'
    COLON,          ///< Colon ':'
    COMMA,          ///< Comma ','
    
    // Special
    END,            ///< End of file marker
    ERROR           ///< For error tokens
};

struct Token {
    TokenType type;     ///< Type of the token
    std::string value;  ///< Actual text of the token
    int line;          ///< Line number in source
    int column;        ///< Column number in source

    Token(TokenType t, const std::string& v, int l, int c);
};

#endif // TOKEN_H