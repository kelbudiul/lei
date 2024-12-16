#ifndef SEMANTIC_VISITOR_H
#define SEMANTIC_VISITOR_H

#include "visitor.h"
#include "symbol_table.h"
#include "ast.h"
#include <optional>


class SemanticAnalyzer : public Visitor {
public:
    SemanticAnalyzer() = default;

    // Entry point for analysis
    bool analyze(Program* program);

    // Visitor interface implementation
    void visit(Program* node) override;
    void visit(FunctionDecl* node) override;
    void visit(NumberExpr* node) override;
    void visit(StringExpr* node) override;
    void visit(BoolExpr* node) override;
    void visit(VariableExpr* node) override;
    void visit(ArrayAccessExpr* node) override;
    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
    void visit(AssignExpr* node) override;
    void visit(CallExpr* node) override;
    void visit(ArrayInitExpr* node) override;
    void visit(ArrayAllocExpr* node) override;
    void visit(ExprStmt* node) override;
    void visit(VarDeclStmt* node) override;
    void visit(BlockStmt* node) override;
    void visit(IfStmt* node) override;
    void visit(WhileStmt* node) override;
    void visit(ReturnStmt* node) override;
    void visit(TypeExpr* node) override;

private:
    SymbolTable symbolTable;
    Type currentFunctionReturnType{"void"};  // Track return type for validation
    bool mainFound = false;
    bool validateMainFunction(FunctionDecl* func);
    bool isValidMainSignature(FunctionDecl* func);
    // Type checking helpers
    std::optional<Type> getExprType(Expr* expr);
    bool checkBinaryOperatorTypes(const Token& op, const Type& left, const Type& right);
    bool checkUnaryOperatorTypes(const Token& op, const Type& operand);
    void ensureArrayType(const Type& type, const Token& context);
    void ensureNumericType(const Type& type, const Token& context);
    void ensureBooleanType(const Type& type, const Token& context);
    bool isConditionExpr(Expr* expr);
};

#endif // SEMANTIC_VISITOR_H