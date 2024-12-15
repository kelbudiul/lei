#include "ast.h"
#include "visitor.h"


void Program::accept(Visitor* visitor) {
    visitor->visit(this);
}

void FunctionDecl::accept(Visitor* visitor) {
    visitor->visit(this);
}

void NumberExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void StringExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BoolExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void VariableExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayAccessExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BinaryExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void UnaryExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void AssignExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void CallExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayInitExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayAllocExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ExprStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void VarDeclStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BlockStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void IfStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void WhileStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ReturnStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void TypeExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}