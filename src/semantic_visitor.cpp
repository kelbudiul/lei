#include "semantic_visitor.h"

SemanticVisitor::SemanticVisitor(SymbolTable& table) : symbolTable(table) {}

void SemanticVisitor::visit(ASTNode* node) {
    // Generic node visit - can be used for logging or default behavior
}

void SemanticVisitor::visit(FunctionAST* node) {
    // Check function declaration
    if (symbolTable.exists(node->name)) {
        errors.push_back("Function " + node->name + " already declared");
    }
    symbolTable.add(node->name, node->returnType);

    // Recursively visit body
    node->body->accept(this);
}

void SemanticVisitor::visit(ReturnAST* node) {
    // Type checking
    if (std::holds_alternative<int>(node->value)) {
        // Perform int-specific checks
    } else if (std::holds_alternative<float>(node->value)) {
        // Perform float-specific checks
    }
}

bool SemanticVisitor::hasErrors() const {
    return !errors.empty();
}

std::vector<std::string> SemanticVisitor::getErrors() const {
    return errors;
}