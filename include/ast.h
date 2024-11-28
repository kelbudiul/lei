#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include <string>

// Forward Declarations
class ASTVisitor;
class ASTNode;
class ProgramNode;
class FunctionDeclarationNode;
class BinaryOperationNode;
class LiteralNode;
class IdentifierNode;

// Visitable Interface
class Visitable {
public:
    virtual void accept(ASTVisitor* visitor) = 0;
    virtual ~Visitable() = default;
};

// Visitor Interface
class ASTVisitor {
public:
    virtual void visit(ProgramNode* node) = 0;
    virtual void visit(FunctionDeclarationNode* node) = 0;
    virtual void visit(BinaryOperationNode* node) = 0;
    virtual void visit(LiteralNode* node) = 0;
    virtual void visit(IdentifierNode* node) = 0;
    virtual ~ASTVisitor() = default;
};

// Base AST Node
class ASTNode : public Visitable {
public:
    virtual ~ASTNode() = default;
};

// Program Node
class ProgramNode : public ASTNode {
private:
    std::vector<std::unique_ptr<ASTNode>> declarations;

public:
    void addDeclaration(std::unique_ptr<ASTNode> node);
    void accept(ASTVisitor* visitor) override;
    const std::vector<std::unique_ptr<ASTNode>>& getDeclarations() const;
};

// Function Declaration Node
class FunctionDeclarationNode : public ASTNode {
private:
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<ASTNode> body;

public:
    FunctionDeclarationNode(
        std::string name, 
        std::vector<std::string> params, 
        std::unique_ptr<ASTNode> body
    );

    void accept(ASTVisitor* visitor) override;
    const std::string& getName() const;
    const std::vector<std::string>& getParameters() const;
    ASTNode* getBody() const;
};

// Binary Operation Node
class BinaryOperationNode : public ASTNode {
private:
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::string operation;

public:
    BinaryOperationNode(
        std::unique_ptr<ASTNode> left, 
        std::string op, 
        std::unique_ptr<ASTNode> right
    );

    void accept(ASTVisitor* visitor) override;
    ASTNode* getLeft() const;
    ASTNode* getRight() const;
    const std::string& getOperation() const;
};

// Literal Node
class LiteralNode : public ASTNode {
private:
    std::string value;

public:
    explicit LiteralNode(std::string val);
    void accept(ASTVisitor* visitor) override;
    const std::string& getValue() const;
};

// Identifier Node
class IdentifierNode : public ASTNode {
private:
    std::string name;

public:
    explicit IdentifierNode(std::string id);
    void accept(ASTVisitor* visitor) override;
    const std::string& getName() const;
};

// Concrete Visitors
class PrintVisitor : public ASTVisitor {
public:
    void visit(ProgramNode* node) override;
    void visit(FunctionDeclarationNode* node) override;
    void visit(BinaryOperationNode* node) override;
    void visit(LiteralNode* node) override;
    void visit(IdentifierNode* node) override;
};

// AST Builder
class ASTBuilder {
public:
    static std::unique_ptr<ProgramNode> buildSampleAST();
};

#endif // AST_H