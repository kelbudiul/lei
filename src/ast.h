#ifndef AST_H
#define AST_H

#include "../include/common.h"
#include "visitor.h"



// Base AST Node
struct ASTNode: Visitable {
    virtual ~ASTNode() = default;

    // Implement accept method
    void accept(Visitor* visitor) override {
        visitor->visit(this);
    }
};


struct ExpressionAST : ASTNode {
    Type type = Type::Unknown;
    virtual Type getType() { return type; }
};


struct BinaryExpressionAST : ExpressionAST {
    std::string op;
    std::unique_ptr<ExpressionAST> left;
    std::unique_ptr<ExpressionAST> right;

    BinaryExpressionAST(std::string op,
                       std::unique_ptr<ExpressionAST> left,
                       std::unique_ptr<ExpressionAST> right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

    void accept(Visitor* visitor) override {
        visitor->visit(this);
    }
};

struct NumberExpressionAST : ExpressionAST {
    int value;

    explicit NumberExpressionAST(int value) : value(value) {
        type = Type::Int;
    }

    void accept(Visitor* visitor) override {
        visitor->visit(this);
    }
};


// Function AST Node
struct FunctionAST : ASTNode {
    std::string name;
    std::string returnType;
    std::unique_ptr<ASTNode> body;

    FunctionAST(const std::string &name, 
                const std::string &returnType, 
                std::unique_ptr<ASTNode> body);

    void accept(Visitor* visitor) override {
        visitor->visit(this);
    }
};

// Return Statement AST Node
struct ReturnAST : ASTNode {
    ReturnValue value;

    ReturnAST(int v);
    ReturnAST(float f);
    ReturnAST(const std::string &s);

    void accept(Visitor* visitor) override {
        visitor->visit(this);
    }
};

struct VariableDeclarationAST : ASTNode {
    std::string name;
    Type declaredType;
    std::unique_ptr<ExpressionAST> initializer;

    VariableDeclarationAST(const std::string &name, 
                          Type type,
                          std::unique_ptr<ExpressionAST> init)
        : name(name), declaredType(type), initializer(std::move(init)) {}
};
// Assignment Statement AST Node

struct AssignmentAST: ASTNode {};

struct ArrayAST: ASTNode {};

#endif // AST_H