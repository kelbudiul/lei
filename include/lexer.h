// include/lexer.h
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "token.h"

class Lexer {
private:
    std::string input;
    size_t position = 0;
    int currentLine = 1;
    int currentColumn = 1;

    // Helper methods
    char peek() const;
    char consume();
    void skipWhitespace();
    Token createToken(Token::Type type, const std::string& value);

public:
    // Constructor
    Lexer(const std::string& src);

    // Tokenization method
    std::vector<Token> tokenize();

    // Error handling method
    void reportError(const std::string& message);
};

#endif // LEXER_H