#ifndef VISITOR_H
#define VISITOR_H

// Forward declaration
class ASTNode;
class FunctionAST;
class ReturnAST;
class ExpressionAST;
class BinaryExpressionAST;

class Visitor {
public:
    virtual ~Visitor() = default;
    
    // Pure virtual methods for each AST node type
    virtual void visit(ASTNode* node) = 0;
    virtual void visit(FunctionAST* node) = 0;
    virtual void visit(ReturnAST* node) = 0;
    virtual void visit(BinaryExpressionAST* node) = 0;
    virtual void visit(ExpressionAST* node) = 0;
};

// Base Visitable interface
class Visitable {
public:
    virtual ~Visitable() = default;
    virtual void accept(Visitor* visitor) = 0;
};
#endif // VISITOR_H