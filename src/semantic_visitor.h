#ifndef SEMANTIC_VISITOR_H
#define SEMANTIC_VISITOR_H

#include "symbol_table.h"
#include "visitor.h"
#include "ast.h"

class SemanticVisitor : public Visitor {
private:
    SymbolTable& symbolTable;
    std::vector<std::string> errors;
    
    // Current function context
    std::string currentFunction;
    Type currentReturnType;
    
    void error(const std::string& message) {
        errors.push_back(message);
    }
    
    bool isAssignable(Type from, Type to);
    bool isComparable(Type left, Type right);

public:
    SemanticVisitor(SymbolTable& table);
    
    // Visit methods for all node types
    void visit(ASTNode* node) override;
    void visit(FunctionAST* node) override;
    void visit(BlockAST* node) override;
    void visit(IfStatementAST* node) override;
    void visit(WhileStatementAST* node) override;
    void visit(ReturnAST* node) override;
    void visit(VariableDeclarationAST* node) override;
    void visit(BinaryExprAST* node) override;
    void visit(UnaryExprAST* node) override;
    void visit(LiteralAST* node) override;
    void visit(VariableExprAST* node) override;
    void visit(CallExprAST* node) override;
    void visit(AssignmentExprAST* node) override;
    
    bool hasErrors() const;
    const std::vector<std::string>& getErrors() const;
};

#endif // SEMANTIC_VISITOR_H