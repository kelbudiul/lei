#ifndef PARSER_H
#define PARSER_H

#include "../include/common.h"
#include "ast.h"
#include "lexer.h"

// Parsing class
class Parser {
private:
    std::vector<Token> tokens;
    size_t index = 0;

    // Helper methods
    Token currentToken();
    void consume(TokenType type);

public:
    explicit Parser(const std::vector<Token> &tokens);
    
    // Parse a function definition
    std::unique_ptr<FunctionAST> parseFunction();
    
    // Parse a return statement
    std::unique_ptr<ReturnAST> parseReturn();
};

#endif // PARSER_H