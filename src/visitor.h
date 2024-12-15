#ifndef VISITOR_H
#define VISITOR_H

// Forward declarations of all AST node types
class Program;
class FunctionDecl;
class NumberExpr;
class StringExpr;
class BoolExpr;
class VariableExpr;
class ArrayAccessExpr;
class BinaryExpr;
class UnaryExpr;
class AssignExpr;
class CallExpr;
class ArrayInitExpr;
class ArrayAllocExpr;
class TypeExpr;
class ExprStmt;
class VarDeclStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ReturnStmt;

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(Program* node) = 0;
    virtual void visit(FunctionDecl* node) = 0;
    virtual void visit(NumberExpr* node) = 0;
    virtual void visit(StringExpr* node) = 0;
    virtual void visit(BoolExpr* node) = 0;
    virtual void visit(VariableExpr* node) = 0;
    virtual void visit(ArrayAccessExpr* node) = 0;
    virtual void visit(BinaryExpr* node) = 0;
    virtual void visit(UnaryExpr* node) = 0;
    virtual void visit(AssignExpr* node) = 0;
    virtual void visit(CallExpr* node) = 0;
    virtual void visit(ArrayInitExpr* node) = 0;
    virtual void visit(ArrayAllocExpr* node) = 0;
    virtual void visit(TypeExpr* node) = 0;
    virtual void visit(ExprStmt* node) = 0;
    virtual void visit(VarDeclStmt* node) = 0;
    virtual void visit(BlockStmt* node) = 0;
    virtual void visit(IfStmt* node) = 0;
    virtual void visit(WhileStmt* node) = 0;
    virtual void visit(ReturnStmt* node) = 0;

};

#endif // VISITOR_H