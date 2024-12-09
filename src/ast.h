#ifndef AST_H
#define AST_H

#include "../include/common.h"

// Base AST Node
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Function AST Node
struct FunctionAST : ASTNode {
    std::string name;
    std::string returnType;
    std::unique_ptr<ASTNode> body;

    FunctionAST(const std::string &name, 
                const std::string &returnType, 
                std::unique_ptr<ASTNode> body);
};

// Return Statement AST Node
struct ReturnAST : ASTNode {
    ReturnValue value;

    ReturnAST(int v);
    ReturnAST(float f);
    ReturnAST(const std::string &s);
};

#endif // AST_H