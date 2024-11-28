// src/lexer.cpp
#include "lexer.h"
#include <stdexcept>
#include <cctype>
#include "token.h"

Lexer::Lexer(const std::string& src) : input(src) {}

char Lexer::peek() const {
    return (position < input.length()) ? input[position] : '\0';
}

char Lexer::consume() {
    char current = peek();
    
    if (current == '\n') {
        currentLine++;
        currentColumn = 1;
    } else {
        currentColumn++;
    }
    
    if (position < input.length()) position++;
    return current;
}

void Lexer::skipWhitespace() {
    while (std::isspace(peek())) consume();
}

Token Lexer::createToken(Token::Type type, const std::string& value) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = currentLine;
    token.column = currentColumn;
    return token;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (peek() != '\0') {
        // Skip whitespace
        skipWhitespace();
        
        char current = peek();
        
        // Parse integers
        if (std::isdigit(current)) {
            std::string number;
            while (std::isdigit(peek())) {
                number += consume();
            }
            tokens.push_back(createToken(Token::INTEGER, number));
            continue;
        }
        
        // Parse identifiers
        if (std::isalpha(current)) {
            std::string identifier;
            while (std::isalnum(peek())) {
                identifier += consume();
            }
            tokens.push_back(createToken(Token::IDENTIFIER, identifier));
            continue;
        }
        
        // Parse operators and punctuation
        switch (current) {
            case '+': 
                tokens.push_back(createToken(Token::PLUS, "+")); 
                consume(); 
                break;
            case '-': 
                tokens.push_back(createToken(Token::MINUS, "-")); 
                consume(); 
                break;
            case '*': 
                tokens.push_back(createToken(Token::MULTIPLY, "*")); 
                consume(); 
                break;
            case '/': 
                tokens.push_back(createToken(Token::DIVIDE, "/")); 
                consume(); 
                break;
            case '(': 
                tokens.push_back(createToken(Token::LPAREN, "(")); 
                consume(); 
                break;
            case ')': 
                tokens.push_back(createToken(Token::RPAREN, ")")); 
                consume(); 
                break;
            default:
                reportError("Unexpected character: " + std::string(1, current));
                consume();
        }
    }
    
    // Add end of token stream
    tokens.push_back(createToken(Token::END, ""));
    return tokens;
}

void Lexer::reportError(const std::string& message) {
    std::string errorMsg = "Lexical Error at line " + 
        std::to_string(currentLine) + ", column " + 
        std::to_string(currentColumn) + ": " + message;
    throw std::runtime_error(errorMsg);
}