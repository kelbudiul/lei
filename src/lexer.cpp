// src/lexer.cpp
#include <stdexcept>
#include <cctype>
#include "token.h"
#include "lexer.h"


namespace Lei
{
        
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

    Token Lexer::createToken(TokenType type, const std::string& value, int currentLine, int currentColumn) {
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
                tokens.push_back(createToken(TokenType::INTEGER_LITERAL, number, currentLine, currentColumn));
                continue;
            }
            
            // Parse identifiers
            if (std::isalpha(current)) {
                std::string identifier;
                while (std::isalnum(peek())) {
                    identifier += consume();
                }
                tokens.push_back(createToken(TokenType::IDENTIFIER, identifier, currentLine, currentColumn));
                continue;
            }
            
            // Parse operators and punctuation
            switch (current) {
                case '+': 
                    tokens.push_back(createToken(TokenType::PLUS, "+", currentLine, currentColumn)); 
                    consume(); 
                    break;
                case '-': 
                    tokens.push_back(createToken(TokenType::MINUS, "-", currentLine, currentColumn)); 
                    consume(); 
                    break;
                case '*': 
                    tokens.push_back(createToken(TokenType::MULTIPLY, "*", currentLine, currentColumn)); 
                    consume(); 
                    break;
                case '/': 
                    tokens.push_back(createToken(TokenType::DIVIDE, "/", currentLine, currentColumn)); 
                    consume(); 
                    break;
                case '(': 
                    tokens.push_back(createToken(TokenType::LEFT_PARENTHESIS, "(", currentLine, currentColumn)); 
                    consume(); 
                    break;
                case ')': 
                    tokens.push_back(createToken(TokenType::RIGHT_PARENTHESIS, ")", currentLine, currentColumn)); 
                    consume(); 
                    break;
                default:
                    reportError("Unexpected character: " + std::string(1, current));
                    consume();
            }
        }
        
        // Add end of token stream
        tokens.push_back(createToken(TokenType::END_OF_FILE, "", currentLine, currentColumn));
        return tokens;
    }

    void Lexer::reportError(const std::string& message) {
        std::string errorMsg = "Lexical Error at line " + 
            std::to_string(currentLine) + ", column " + 
            std::to_string(currentColumn) + ": " + message;
        throw std::runtime_error(errorMsg);
    }
} // namespace Lei
