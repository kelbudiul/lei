#include "semantic_visitor.h"

SemanticVisitor::SemanticVisitor(SymbolTable& table) 
    : symbolTable(table), currentFunction(""), currentReturnType(Type::Void) {}

bool SemanticVisitor::isAssignable(Type from, Type to) {
    if (from == to) return true;
    
    // Allow int to float conversion
    if (from == Type::Int && to == Type::Float) return true;
    
    return false;
}

bool SemanticVisitor::isComparable(Type left, Type right) {
    if (left == right) return true;
    
    // Allow comparing ints and floats
    if ((left == Type::Int && right == Type::Float) ||
        (left == Type::Float && right == Type::Int)) return true;
    
    return false;
}

void SemanticVisitor::visit(FunctionAST* node) {
    // Check for duplicate function
    if (symbolTable.exists(node->name)) {
        error("Function " + node->name + " already declared");
        return;
    }
    
    // Add function to symbol table
    FunctionSymbol func;
    func.name = node->name;
    func.returnType = stringToType(node->returnType);
    
    for (const auto& param : node->parameters) {
        func.paramTypes.push_back(stringToType(param.second));
    }
    
    symbolTable.addFunction(func);
    
    // Set up function context
    currentFunction = node->name;
    currentReturnType = func.returnType;
    
    // Create new scope for parameters
    symbolTable.enterScope();
    
    // Add parameters to symbol table
    for (const auto& param : node->parameters) {
        symbolTable.addVariable(param.first, stringToType(param.second));
    }
    
    // Visit function body
    node->body->accept(this);
    
    // Clean up
    symbolTable.exitScope();
    currentFunction = "";
    currentReturnType = Type::Void;
}

void SemanticVisitor::visit(BlockAST* node) {
    symbolTable.enterScope();
    
    for (const auto& stmt : node->statements) {
        stmt->accept(this);
    }
    
    symbolTable.exitScope();
}

void SemanticVisitor::visit(IfStatementAST* node) {
    // Check condition type
    node->condition->accept(this);
    if (node->condition->getType() != Type::Bool) {
        error("If condition must be boolean");
    }
    
    // Visit branches
    node->thenBlock->accept(this);
    if (node->elseBlock) {
        node->elseBlock->accept(this);
    }
}

void SemanticVisitor::visit(VariableDeclarationAST* node) {
    // Check if variable name already exists in current scope
    if (symbolTable.existsInCurrentScope(node->name)) {
        error("Variable " + node->name + " already declared in this scope");
        return;
    }

    Type declaredType = stringToType(node->type);
    
    // If there's an initializer, check its type
    if (node->initializer) {
        node->initializer->accept(this);
        Type initType = node->initializer->getType();
        
        if (!isAssignable(initType, declaredType)) {
            error("Cannot initialize variable of type " + 
                  typeToString(declaredType) + " with value of type " + 
                  typeToString(initType));
            return;
        }
    }

    // Add to symbol table
    symbolTable.addVariable(node->name, declaredType);
}

bool SemanticVisitor::hasErrors() const {
    return !errors.empty();
}

std::vector<std::string> SemanticVisitor::getErrors() const {
    return errors;
}