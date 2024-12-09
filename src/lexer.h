#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include "../include/common.h"

struct Token {
        TokenType type;
        std::string value;
        int line;
        int column;

        Token(TokenType t, const std::string& v, int l, int c);
    };

class Lexer {
    const std::string& input;
    size_t pos;
    int line;
    int column;

    void advance();

public:
    explicit Lexer(const std::string& code);

    std::vector<Token> tokenize();
};
#endif // LEXER_H