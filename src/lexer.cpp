/**
 * @file lexer.cpp
 * @brief Implementation of the Lexer class
 */

#include "lexer.h"
#include <cctype>
#include <sstream>
#include <algorithm>
#include <unordered_map>

// Constructor implementation for LexerError
LexerError::LexerError(const std::string& message, int l, int c, const std::string& d)
    : std::runtime_error(formatMessage(message, l, c, d))
    , line(l)
    , column(c)
    , details(d) {}

std::string LexerError::formatMessage(const std::string& message, int line, int column, const std::string& details) {
    std::stringstream ss;
    ss << "Lexer error at line " << line << ", column " << column << ": " << message;
    if (!details.empty()) {
        ss << "\nDetails: " << details;
    }
    return ss.str();
}

// Keyword mapping
const std::unordered_map<std::string, TokenType> keywords = {
    {"fn", FN},
    {"int", INT},
    {"float", FLOAT_TYPE},
    {"bool", BOOL_TYPE},
    {"str", STRING_TYPE},
    {"var", VAR},
    {"return", RETURN},
    {"if", IF},
    {"else", ELSE},
    {"while", WHILE},
    {"true", BOOL_LITERAL},
    {"false", BOOL_LITERAL}
};

// Token constructor
Token::Token(TokenType t, const std::string& v, int l, int c)
    : type(t), value(v), line(l), column(c) {}

// Lexer constructor
Lexer::Lexer(const std::string& code)
    : input(code), pos(0), line(1), column(1) {}

char Lexer::peek() const {
    return pos < input.size() ? input[pos] : '\0';
}

char Lexer::peekNext() const {
    return (pos + 1) < input.size() ? input[pos + 1] : '\0';
}

void Lexer::advance() {
    if (pos < input.size()) {
        if (input[pos] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++;
    }
}

std::string Lexer::getCurrentContext() const {
    const size_t contextSize = 20;  // Changed to size_t
    size_t start = (pos > contextSize) ? pos - contextSize : 0;
    size_t length = std::min(contextSize * 2, input.size() - start);
    std::string context = input.substr(start, length);
    
    // Add position marker
    if (pos - start < context.length()) {
        context += "\n" + std::string(pos - start, ' ') + "^";
    }
    return context;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos < input.size()) {
        char current = peek();
        
        if (std::isspace(current)) {
            advance();
        }
        else if (current == '/' && peekNext() == '/') {
            // Skip until end of line
            while (pos < input.size() && peek() != '\n') {
                advance();
            }
            if (pos < input.size()) {
                advance(); // Skip the newline
            }
        }
        else {
            break;
        }
    }
}

Token Lexer::handleIdentifier() {
    std::string word;
    int startLine = line;
    int startColumn = column;
    
    // First character is already verified to be alpha or underscore
    while (pos < input.size() && (std::isalnum(peek()) || peek() == '_')) {
        word += peek();
        advance();
    }
    
    // Check if it's a keyword
    auto it = keywords.find(word);
    if (it != keywords.end()) {
        return Token(it->second, word, startLine, startColumn);
    }
    
    return Token(IDENTIFIER, word, startLine, startColumn);
}

Token Lexer::handleNumber() {
    std::string num;
    int startLine = line;
    int startColumn = column;
    bool isFloat = false;
    bool hasDigitsAfterDot = false;
    
    while (pos < input.size() && (std::isdigit(peek()) || peek() == '.')) {
        if (peek() == '.') {
            if (isFloat) {
                throw LexerError("Invalid number format: multiple decimal points",
                               line, column, "Number so far: " + num);
            }
            if (num.empty()) {
                num = "0";  // Add leading zero for .123 format
            }
            isFloat = true;
            num += peek();
            advance();
            
            if (!std::isdigit(peek())) {
                throw LexerError("Invalid float literal: needs at least one digit after decimal point",
                               line, column, "Number so far: " + num);
            }
        } else {
            if (isFloat) {
                hasDigitsAfterDot = true;
            }
            num += peek();
            advance();
        }
    }
    
    // Handle trailing decimal point
    if (isFloat && !hasDigitsAfterDot) {
        throw LexerError("Invalid float literal: needs at least one digit after decimal point",
                        line, column, "Number so far: " + num);
    }
    
    return Token(isFloat ? FLOAT_LITERAL : NUMBER, num, startLine, startColumn);
}

Token Lexer::handleString() {
    std::string str;
    int startLine = line;
    int startColumn = column;
    
    advance(); // Skip opening quote
    
    while (pos < input.size() && peek() != '"') {
        if (peek() == '\\') {
            if (pos + 1 >= input.size()) {
                throw LexerError("Unterminated escape sequence",
                               line, column, "String so far: " + str);
            }
            advance(); // Skip backslash
            switch (peek()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case 'r': str += '\r'; break;
                case '"': str += '"'; break;
                case '\\': str += '\\'; break;
                default:
                    throw LexerError("Invalid escape sequence",
                                   line, column,
                                   "Invalid escape character: \\" + std::string(1, peek()));
            }
        } else if (peek() == '\n') {
            throw LexerError("Unterminated string literal: newline in string",
                           line, column, "String so far: " + str);
        } else {
            str += peek();
        }
        advance();
    }
    
    if (pos >= input.size()) {
        throw LexerError("Unterminated string literal",
                        startLine, startColumn,
                        "String so far: " + str + "\nContext:\n" + getCurrentContext());
    }
    
    advance(); // Skip closing quote
    return Token(STRING_LITERAL, str, startLine, startColumn);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    try {
        while (pos < input.size()) {
            skipWhitespaceAndComments();
            
            if (pos >= input.size()) break;
            
            int startLine = line;
            int startColumn = column;
            char current = peek();
            
            // Handle different token types
            if (std::isalpha(current) || current == '_') {
                tokens.push_back(handleIdentifier());
            }
            else if (std::isdigit(current) || (current == '.' && std::isdigit(peekNext()))) {
                tokens.push_back(handleNumber());
            }
            else if (current == '"') {
                tokens.push_back(handleString());
            }
            else {
                // Handle operators and special characters
                advance();
                switch (current) {
                    case '+':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(PLUS_EQUALS, "+=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(PLUS, "+", startLine, startColumn);
                        }
                        break;
                        
                    case '-':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(MINUS_EQUALS, "-=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(MINUS, "-", startLine, startColumn);
                        }
                        break;
                        
                    case '*':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(STAR_EQUALS, "*=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(STAR, "*", startLine, startColumn);
                        }
                        break;
                        
                    case '/':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(SLASH_EQUALS, "/=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(SLASH, "/", startLine, startColumn);
                        }
                        break;
                        
                    case '=':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(EQUALS_EQUALS, "==", startLine, startColumn);
                        } else {
                            tokens.emplace_back(EQUALS, "=", startLine, startColumn);
                        }
                        break;
                        
                    case '!':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(NOT_EQUALS, "!=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(NOT, "!", startLine, startColumn);
                        }
                        break;
                        
                    case '<':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(LESS_EQUAL, "<=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(LESS, "<", startLine, startColumn);
                        }
                        break;
                        
                    case '>':
                        if (peek() == '=') {
                            advance();
                            tokens.emplace_back(GREATER_EQUAL, ">=", startLine, startColumn);
                        } else {
                            tokens.emplace_back(GREATER, ">", startLine, startColumn);
                        }
                        break;
                        
                    case '&':
                        if (peek() == '&') {
                            advance();
                            tokens.emplace_back(AND, "&&", startLine, startColumn);
                        } else {
                            throw LexerError("Unexpected character", startLine, startColumn,
                                           "Expected '&&' for logical AND operator");
                        }
                        break;
                        
                    case '|':
                        if (peek() == '|') {
                            advance();
                            tokens.emplace_back(OR, "||", startLine, startColumn);
                        } else {
                            throw LexerError("Unexpected character", startLine, startColumn,
                                           "Expected '||' for logical OR operator");
                        }
                        break;
                        
                    // Delimiters
                    case '{': tokens.emplace_back(LBRACE, "{", startLine, startColumn); break;
                    case '}': tokens.emplace_back(RBRACE, "}", startLine, startColumn); break;
                    case '(': tokens.emplace_back(LPAREN, "(", startLine, startColumn); break;
                    case ')': tokens.emplace_back(RPAREN, ")", startLine, startColumn); break;
                    case '[': tokens.emplace_back(LBRACKET, "[", startLine, startColumn); break;
                    case ']': tokens.emplace_back(RBRACKET, "]", startLine, startColumn); break;
                    case ';': tokens.emplace_back(SEMICOLON, ";", startLine, startColumn); break;
                    case ':': tokens.emplace_back(COLON, ":", startLine, startColumn); break;
                    case ',': tokens.emplace_back(COMMA, ",", startLine, startColumn); break;
                    
                    default:
                        throw LexerError("Unexpected character",
                                       startLine, startColumn,
                                       "Character: '" + std::string(1, current) + "'\n" +
                                       "Context:\n" + getCurrentContext());
                }
            }
        }
        
        // Add end token
        tokens.emplace_back(END, "", line, column);
        return tokens;
    }
    catch (const LexerError& e) {
        throw; // Re-throw lexer errors as they already have proper formatting
    }
    catch (const std::exception& e) {
        // Wrap other exceptions with position information
        throw LexerError(e.what(), line, column,
                        "Context:\n" + getCurrentContext());
    }
}