#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "token.h"
#include "ast.h"
#include "error_handler.h"
#include <iostream>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();

private:
    void debugToken(const std::string& context) const {
        std::cout << "DEBUG [" << context << "] - "
                  << "Current token: Type=" << static_cast<int>(peek().type) 
                  << ", Value='" << peek().value 
                  << "', Line=" << peek().line 
                  << ", Column=" << peek().column << "\n";
    }


    const std::vector<Token>& tokens;
    size_t current;

    // Helper methods
    Token peek() const;         // Look at current token without consuming
    Token previous() const;     // Get last consumed token
    Token advance();           // Move to next token and return previous
    bool isAtEnd() const;      // Check if we reached end of tokens
    bool check(TokenType type) const; // Check if current token is of given type
    bool match(TokenType type); // Check and consume token if it matches
    Token consume(TokenType type, const std::string& message);
    void synchronize();

    // Type parsing
    Type parseType();
    
    // Parsing methods following the grammar
    std::unique_ptr<FunctionDecl> parseFunction();
    std::vector<Parameter> parseParameters();
    
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<VarDeclStmt> parseVarDecl();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<ExprStmt> parseExprStmt();
    
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parseCall();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseTypeExpression();
    
    std::unique_ptr<ArrayInitExpr> parseArrayInitializer();
    std::unique_ptr<ArrayAllocExpr> parseArrayAllocation();
    std::vector<std::unique_ptr<Expr>> parseArguments();
};

#endif // PARSER_H