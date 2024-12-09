#include "semantic_analyzer.h"

SemanticAnalyzer::SemanticAnalyzer(SymbolTable &symbols) : symbols(symbols) {}

void SemanticAnalyzer::checkFunction(FunctionAST &func) {
    // Add function to the symbol table
    symbols.add(func.name, func.returnType);

    // Check return type
    auto *returnAST = dynamic_cast<ReturnAST *>(func.body.get());
    if (!returnAST) {
        throw std::runtime_error("Semantic Error: Missing return statement in function " + func.name);
    }
    
    // Type checking based on return type
    if (func.returnType == "int" && !std::holds_alternative<int>(returnAST->value)) {
        throw std::runtime_error("Semantic Error: Return type mismatch in function " + func.name);
    }
}