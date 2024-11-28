#include "ast.h"
#include <iostream>

// ProgramNode Implementation
void ProgramNode::addDeclaration(std::unique_ptr<ASTNode> node) {
    declarations.push_back(std::move(node));
}

void ProgramNode::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

const std::vector<std::unique_ptr<ASTNode>>& ProgramNode::getDeclarations() const {
    return declarations;
}

// FunctionDeclarationNode Implementation
FunctionDeclarationNode::FunctionDeclarationNode(
    std::string name, 
    std::vector<std::string> params, 
    std::unique_ptr<ASTNode> body
) : name(std::move(name)), 
    parameters(std::move(params)), 
    body(std::move(body)) {}

void FunctionDeclarationNode::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

const std::string& FunctionDeclarationNode::getName() const { 
    return name; 
}

const std::vector<std::string>& FunctionDeclarationNode::getParameters() const { 
    return parameters; 
}

ASTNode* FunctionDeclarationNode::getBody() const { 
    return body.get(); 
}

// BinaryOperationNode Implementation
BinaryOperationNode::BinaryOperationNode(
    std::unique_ptr<ASTNode> left, 
    std::string op, 
    std::unique_ptr<ASTNode> right
) : left(std::move(left)), 
    operation(op), 
    right(std::move(right)) {}

void BinaryOperationNode::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

ASTNode* BinaryOperationNode::getLeft() const { 
    return left.get(); 
}

ASTNode* BinaryOperationNode::getRight() const { 
    return right.get(); 
}

const std::string& BinaryOperationNode::getOperation() const { 
    return operation; 
}

// LiteralNode Implementation
LiteralNode::LiteralNode(std::string val) : value(std::move(val)) {}

void LiteralNode::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

const std::string& LiteralNode::getValue() const { 
    return value; 
}

// IdentifierNode Implementation
IdentifierNode::IdentifierNode(std::string id) : name(std::move(id)) {}

void IdentifierNode::accept(ASTVisitor* visitor) {
    visitor->visit(this);
}

const std::string& IdentifierNode::getName() const { 
    return name; 
}

// PrintVisitor Implementation
void PrintVisitor::visit(ProgramNode* node) {
    std::cout << "Program Node:" << std::endl;
    for (const auto& decl : node->getDeclarations()) {
        decl->accept(this);
    }
}

void PrintVisitor::visit(FunctionDeclarationNode* node) {
    std::cout << "Function: " << node->getName() << std::endl;
    
    std::cout << "  Parameters: ";
    for (const auto& param : node->getParameters()) {
        std::cout << param << " ";
    }
    std::cout << std::endl;

    if (node->getBody()) {
        std::cout << "  Body:" << std::endl;
        node->getBody()->accept(this);
    }
}

void PrintVisitor::visit(BinaryOperationNode* node) {
    std::cout << "Binary Operation: " << node->getOperation() << std::endl;
    
    std::cout << "  Left:" << std::endl;
    node->getLeft()->accept(this);

    std::cout << "  Right:" << std::endl;
    node->getRight()->accept(this);
}

void PrintVisitor::visit(LiteralNode* node) {
    std::cout << "Literal: " << node->getValue() << std::endl;
}

void PrintVisitor::visit(IdentifierNode* node) {
    std::cout << "Identifier: " << node->getName() << std::endl;
}

// AST Builder Implementation
std::unique_ptr<ProgramNode> ASTBuilder::buildSampleAST() {
    auto program = std::make_unique<ProgramNode>();

    // Explicitly create unique_ptrs with correct type
    auto functionBody = std::make_unique<BinaryOperationNode>(
        std::unique_ptr<ASTNode>(new IdentifierNode("x")),
        "+",
        std::unique_ptr<ASTNode>(new LiteralNode("5"))
    );

    auto function = std::make_unique<FunctionDeclarationNode>(
        "example_func", 
        std::vector<std::string>{"x"}, 
        std::move(functionBody)
    );

    program->addDeclaration(std::move(function));
    return program;
}