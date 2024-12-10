#ifndef SEMANTIC_VISITOR_H
#define SEMANTIC_VISITOR_H

#include "symbol_table.h"
#include "visitor.h"
#include "ast.h"

class SemanticVisitor : public Visitor {
private:
    SymbolTable& symbolTable;
    std::vector<std::string> errors;

public:
    SemanticVisitor(SymbolTable& table);

    void visit(ASTNode* node) override;
    void visit(FunctionAST* node) override;
    void visit(ReturnAST* node) override;

    bool hasErrors() const;
    std::vector<std::string> getErrors() const;
};



#endif // SEMANTIC_VISITOR_H