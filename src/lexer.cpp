#include "lexer.h"
#include <cctype>
#include <sstream>
#include <algorithm>
#include <unordered_map>

// Keyword mapping
const std::unordered_map<std::string, TokenType> keywords = {
    {"fn", FN},
    {"void", VOID},
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

Token::Token(TokenType t, const std::string& v, int l, int c)
    : type(t), value(v), line(l), column(c) {}

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
    const size_t contextSize = 20;
    size_t start = (pos > contextSize) ? pos - contextSize : 0;
    size_t length = std::min(contextSize * 2, input.size() - start);
    std::string context = input.substr(start, length);
    
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
    
    while (pos < input.size() && (std::isalnum(peek()) || peek() == '_')) {
        word += peek();
        advance();
    }
    
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
                ErrorHandler::instance().error(
                    ErrorLevel::LEXICAL,
                    line, column,
                    "Invalid number format: multiple decimal points found in number '" + num + "'"
                );
                return Token(ERROR, num, startLine, startColumn);
            }
            if (num.empty()) {
                num = "0";
            }
            isFloat = true;
            num += peek();
            advance();
            
            if (!std::isdigit(peek())) {
                ErrorHandler::instance().error(
                    ErrorLevel::LEXICAL,
                    line, column,
                    "Invalid float literal: needs at least one digit after decimal point"
                );
                return Token(ERROR, num, startLine, startColumn);
            }
        } else {
            if (isFloat) {
                hasDigitsAfterDot = true;
            }
            num += peek();
            advance();
        }
    }
    
    if (isFloat && !hasDigitsAfterDot) {
        ErrorHandler::instance().error(
            ErrorLevel::LEXICAL,
            line, column,
            "Invalid float literal: needs at least one digit after decimal point"
        );
        return Token(ERROR, num, startLine, startColumn);
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
                ErrorHandler::instance().error(
                    ErrorLevel::LEXICAL,
                    line, column,
                    "Unterminated escape sequence in string"
                );
                return Token(ERROR, str, startLine, startColumn);
            }
            advance();
            switch (peek()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case 'r': str += '\r'; break;
                case '"': str += '"'; break;
                case '\\': str += '\\'; break;
                default:
                    ErrorHandler::instance().error(
                        ErrorLevel::LEXICAL,
                        line, column,
                        "Invalid escape sequence '\\" + std::string(1, peek()) + "'"
                    );
                    return Token(ERROR, str, startLine, startColumn);
            }
        } else if (peek() == '\n') {
            ErrorHandler::instance().error(
                ErrorLevel::LEXICAL,
                line, column,
                "Unterminated string literal: newline in string"
            );
            return Token(ERROR, str, startLine, startColumn);
        } else {
            str += peek();
        }
        advance();
    }
    
    if (pos >= input.size()) {
        ErrorHandler::instance().error(
            ErrorLevel::LEXICAL,
            startLine, startColumn,
            "Unterminated string literal"
        );
        return Token(ERROR, str, startLine, startColumn);
    }
    
    advance(); // Skip closing quote
    return Token(STRING_LITERAL, str, startLine, startColumn);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos < input.size()) {
        skipWhitespaceAndComments();
        
        if (pos >= input.size()) break;
        
        int startLine = line;
        int startColumn = column;
        char current = peek();
        
        if (std::isalpha(current) || current == '_') {
            tokens.push_back(handleIdentifier());
        }
        else if (std::isdigit(current) || (current == '.' && std::isdigit(peekNext()))) {
            Token token = handleNumber();
            tokens.push_back(token);
            if (token.type == ERROR) continue;
        }
        else if (current == '"') {
            Token token = handleString();
            tokens.push_back(token);
            if (token.type == ERROR) continue;
        }
        else {
            advance();
            Token token(ERROR, "", startLine, startColumn);
            bool validToken = true;
            
            switch (current) {
                case '+':
                    if (peek() == '=') {
                        advance();
                        token = Token(PLUS_EQUALS, "+=", startLine, startColumn);
                    } else {
                        token = Token(PLUS, "+", startLine, startColumn);
                    }
                    break;
                    
                case '-':
                    if (peek() == '=') {
                        advance();
                        token = Token(MINUS_EQUALS, "-=", startLine, startColumn);
                    } else {
                        token = Token(MINUS, "-", startLine, startColumn);
                    }
                    break;
                    
                case '*':
                    if (peek() == '=') {
                        advance();
                        token = Token(STAR_EQUALS, "*=", startLine, startColumn);
                    } else {
                        token = Token(STAR, "*", startLine, startColumn);
                    }
                    break;
                    
                case '/':
                    if (peek() == '=') {
                        advance();
                        token = Token(SLASH_EQUALS, "/=", startLine, startColumn);
                    } else {
                        token = Token(SLASH, "/", startLine, startColumn);
                    }
                    break;
                    
                case '=':
                    if (peek() == '=') {
                        advance();
                        token = Token(EQUALS_EQUALS, "==", startLine, startColumn);
                    } else {
                        token = Token(EQUALS, "=", startLine, startColumn);
                    }
                    break;
                    
                case '!':
                    if (peek() == '=') {
                        advance();
                        token = Token(NOT_EQUALS, "!=", startLine, startColumn);
                    } else {
                        token = Token(NOT, "!", startLine, startColumn);
                    }
                    break;
                    
                case '<':
                    if (peek() == '=') {
                        advance();
                        token = Token(LESS_EQUAL, "<=", startLine, startColumn);
                    } else {
                        token = Token(LESS, "<", startLine, startColumn);
                    }
                    break;
                    
                case '>':
                    if (peek() == '=') {
                        advance();
                        token = Token(GREATER_EQUAL, ">=", startLine, startColumn);
                    } else {
                        token = Token(GREATER, ">", startLine, startColumn);
                    }
                    break;
                    
                case '&':
                    if (peek() == '&') {
                        advance();
                        token = Token(AND, "&&", startLine, startColumn);
                    } else {
                        ErrorHandler::instance().error(
                            ErrorLevel::LEXICAL,
                            startLine, startColumn,
                            "Expected '&&' for logical AND operator"
                        );
                        validToken = false;
                    }
                    break;
                    
                case '|':
                    if (peek() == '|') {
                        advance();
                        token = Token(OR, "||", startLine, startColumn);
                    } else {
                        ErrorHandler::instance().error(
                            ErrorLevel::LEXICAL,
                            startLine, startColumn,
                            "Expected '||' for logical OR operator"
                        );
                        validToken = false;
                    }
                    break;
                    
                case '{': token = Token(LBRACE, "{", startLine, startColumn); break;
                case '}': token = Token(RBRACE, "}", startLine, startColumn); break;
                case '(': token = Token(LPAREN, "(", startLine, startColumn); break;
                case ')': token = Token(RPAREN, ")", startLine, startColumn); break;
                case '[': token = Token(LBRACKET, "[", startLine, startColumn); break;
                case ']': token = Token(RBRACKET, "]", startLine, startColumn); break;
                case ';': token = Token(SEMICOLON, ";", startLine, startColumn); break;
                case ':': token = Token(COLON, ":", startLine, startColumn); break;
                case ',': token = Token(COMMA, ",", startLine, startColumn); break;
                    
                default:
                    ErrorHandler::instance().error(
                        ErrorLevel::LEXICAL,
                        startLine, startColumn,
                        "Unexpected character '" + std::string(1, current) + "'"
                    );
                    validToken = false;
            }
            
            if (validToken) {
                tokens.push_back(token);
            }
        }
    }
    
    tokens.emplace_back(END, "", line, column);
    return tokens;
}