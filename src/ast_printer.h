#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "visitor.h"
#include <sstream>
#include <string>
#include "ast.h"

class ASTPrinter : public Visitor {
public:
    // Print the entire AST starting from any node
    std::string print(ASTNode* node);

    // Single visit method overloaded for each node type
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
    std::stringstream output;
    int indent = 0;

    void writeLine(const std::string& text);
    std::string formatType(const Type& type);
};

#endif // AST_PRINTER_H