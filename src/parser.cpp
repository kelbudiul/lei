#include "parser.h"

Parser::Parser(const std::vector<Token> &tokens) : tokens(tokens) {}

Token Parser::currentToken() { 
    return tokens[index]; 
}

void Parser::consume(TokenType type) {
    if (currentToken().type != type) 
        throw std::runtime_error("Unexpected token");
    index++;
}

std::unique_ptr<FunctionAST> Parser::parseFunction() {
    consume(FN);
    auto returnType = currentToken().value;
    consume(INT);
    auto name = currentToken().value;
    consume(IDENTIFIER);
    consume(LPAREN);
    consume(RPAREN);
    consume(LBRACE);
    auto body = parseReturn();
    consume(RBRACE);
    return std::make_unique<FunctionAST>(FunctionAST{name, returnType, std::move(body)});
}

std::unique_ptr<ReturnAST> Parser::parseReturn() {
    consume(RETURN);
    int value = std::stoi(currentToken().value);
    consume(NUMBER);
    consume(SEMICOLON);
    return std::make_unique<ReturnAST>(ReturnAST{value});
}