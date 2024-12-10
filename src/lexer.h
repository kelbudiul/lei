/**
 * @file lexer.h
 * @brief Header file for the Lexer class that tokenizes source code.
 * 
 * This lexer supports:
 * - Function declarations with return types (fn keyword)
 * - Type annotations (int, float, bool, str)
 * - Variables and arrays
 * - Control flow (if, else, while)
 * - Operators and assignments
 * - String literals with escape sequences
 * - Single-line comments
 */

#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include <string>
#include <stdexcept>

/**
 * @brief Enumeration of all possible token types
 */
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
    END            ///< End of file marker
};

/**
 * @brief Custom exception class for lexer errors
 */
class LexerError : public std::runtime_error {
    int line;       ///< Line number where the error occurred
    int column;     ///< Column number where the error occurred
    std::string details; ///< Additional error details

public:
    /**
     * @brief Construct a new Lexer Error
     * 
     * @param message Main error message
     * @param line Line number where the error occurred
     * @param column Column number where the error occurred
     * @param details Additional error details (optional)
     */
    LexerError(const std::string& message, int line, int column, const std::string& details = "");

    /**
     * @brief Format the error message with line and column information
     */
    static std::string formatMessage(const std::string& message, int line, int column, const std::string& details);

    /**
     * @brief Get the line number where the error occurred
     */
    int getLine() const { return line; }

    /**
     * @brief Get the column number where the error occurred
     */
    int getColumn() const { return column; }

    /**
     * @brief Get additional error details
     */
    const std::string& getDetails() const { return details; }
};

/**
 * @brief Structure representing a single token
 */
struct Token {
    TokenType type;     ///< Type of the token
    std::string value;  ///< Actual text of the token
    int line;          ///< Line number in source
    int column;        ///< Column number in source

    /**
     * @brief Construct a new Token
     * 
     * @param t Token type
     * @param v Token value
     * @param l Line number
     * @param c Column number
     */
    Token(TokenType t, const std::string& v, int l, int c);
};

/**
 * @brief Lexical analyzer class
 */
class Lexer {
private:
    const std::string& input;  ///< Input source code
    size_t pos;               ///< Current position in input
    int line;                ///< Current line number
    int column;              ///< Current column number

    /**
     * @brief Peek at the current character without consuming it
     */
    char peek() const;

    /**
     * @brief Peek at the next character without consuming it
     */
    char peekNext() const;

    /**
     * @brief Advance to the next character
     */
    void advance();

    /**
     * @brief Skip whitespace and comments
     */
    void skipWhitespaceAndComments();

    /**
     * @brief Handle identifier tokens
     */
    Token handleIdentifier();

    /**
     * @brief Handle numeric literals
     */
    Token handleNumber();

    /**
     * @brief Handle string literals
     */
    Token handleString();

    /**
     * @brief Get the current context for error reporting
     */
    std::string getCurrentContext() const;

public:
    /**
     * @brief Construct a new Lexer
     * 
     * @param code Source code to tokenize
     */
    explicit Lexer(const std::string& code);

    /**
     * @brief Tokenize the input source code
     * 
     * @return Vector of tokens
     * @throws LexerError if an error occurs during tokenization
     */
    std::vector<Token> tokenize();
};

#endif // LEXER_H