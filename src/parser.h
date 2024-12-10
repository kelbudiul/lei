#ifndef PARSER_H
#define PARSER_H

#include "../include/common.h"
#include "ast.h"
#include "lexer.h"

class Parser {
private:
    std::vector<Token> tokens;
    size_t index = 0;

    // Helper methods
    Token currentToken();
    Token peek();
    bool match(TokenType type);
    void consume(TokenType type);
    bool check(TokenType type);
    
    // Parsing methods
    std::unique_ptr<ExpressionAST> parseExpression();
    std::unique_ptr<ExpressionAST> parseAssignment();
    std::unique_ptr<ExpressionAST> parseLogicOr();
    std::unique_ptr<ExpressionAST> parseLogicAnd();
    std::unique_ptr<ExpressionAST> parseEquality();
    std::unique_ptr<ExpressionAST> parseComparison();
    std::unique_ptr<ExpressionAST> parseTerm();
    std::unique_ptr<ExpressionAST> parseFactor();
    std::unique_ptr<ExpressionAST> parseUnary();
    std::unique_ptr<ExpressionAST> parsePrimary();
    std::unique_ptr<CallExprAST> parseCall();
    
    std::unique_ptr<StatementAST> parseStatement();
    std::unique_ptr<BlockAST> parseBlock();
    std::unique_ptr<IfStatementAST> parseIf();
    std::unique_ptr<WhileStatementAST> parseWhile();
    std::unique_ptr<ReturnAST> parseReturn();
    std::unique_ptr<VariableDeclarationAST> parseVarDecl();
    
    std::unique_ptr<FunctionAST> parseFunction();
    std::vector<std::pair<std::string, std::string>> parseParameters();

public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<FunctionAST> parse();
};

#endif // PARSER_H