#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include <string>
#include <stdexcept>
#include "error_handler.h"

#include "token.h"

class Lexer {
private:
    const std::string& input;  ///< Input source code
    size_t pos;               ///< Current position in input
    int line;                ///< Current line number
    int column;              ///< Current column number


    char peek() const;
    char peekNext() const;
    void advance();
    void skipWhitespaceAndComments();
    Token handleIdentifier();
    Token handleNumber();
    Token handleString();  
    std::string getCurrentContext() const;

public:
    explicit Lexer(const std::string& code);
    std::vector<Token> tokenize();
};

#endif // LEXER_H