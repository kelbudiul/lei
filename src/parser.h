#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "token.h"
#include "ast.h"
#include "error_handler.h"

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();

private:
    const std::vector<Token>& tokens;
    size_t current;

    // Token handling
    Token peek() const;
    Token previous() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);
    void synchronize();

    // Type parsing
    Type parseType();
    
    // Declarations and statements
    std::unique_ptr<FunctionDecl> parseFunction();
    std::vector<Parameter> parseParameters();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<VarDeclStmt> parseVarDecl();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<ExprStmt> parseExprStmt();
    
    // Expressions
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
    
    // Array handling
    std::unique_ptr<ArrayInitExpr> parseArrayInitializer();
    std::unique_ptr<ArrayAllocExpr> parseArrayAllocation(const Type& elementType);
    std::vector<std::unique_ptr<Expr>> parseArguments();

    // Expression creation helpers
    std::unique_ptr<Expr> createBinaryExpr(std::unique_ptr<Expr> left,
                                         const Token& op,
                                         std::unique_ptr<Expr> right);
    std::unique_ptr<Expr> createCallExpr(const Token& name,
                                       std::vector<std::unique_ptr<Expr>> args);
    std::unique_ptr<Expr> createAssignExpr(std::unique_ptr<Expr> target,
                                         const Token& op,
                                         std::unique_ptr<Expr> value);

    // Error handling and validation
    void error(const std::string& message);
    void errorAt(const Token& token, const std::string& message);
    bool isAtExpressionEnd() const;
    bool isExpressionStart() const;
    void expectStatementEnd(const std::string& context);
    void checkBalancedDelimiter(TokenType opening, TokenType closing,
                               const std::string& context);

    // Generic parsing helpers
    template<typename T>
    std::vector<T> parseCommaSequence(std::function<T()> parseItem,
                                    TokenType endToken,
                                    const std::string& endTokenStr);
};

#endif // PARSER_H