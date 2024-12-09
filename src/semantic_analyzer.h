#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../include/common.h"
#include "ast.h"
#include "symbol_table.h"

// Semantic analysis class
class SemanticAnalyzer {
private:
    SymbolTable &symbols;

public:
    explicit SemanticAnalyzer(SymbolTable &symbols);

    // Perform semantic checks on a function
    void checkFunction(FunctionAST &func);
};

#endif // SEMANTIC_ANALYZER_H