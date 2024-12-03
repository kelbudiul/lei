#pragma once

#include <string>
#include <vector>
#include "include/token.h"

namespace Lei
{
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
        Token createToken(TokenType type, const std::string& value, int currentLine, int currentColumn);

    public:
        // Constructor
        Lexer(const std::string& src);

        // Tokenization method
        std::vector<Token> tokenize();

        // Error handling method
        void reportError(const std::string& message);
    };

} // namespace Lei
