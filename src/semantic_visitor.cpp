#include "semantic_visitor.h"
#include "error_handler.h"

// isConditionExpr() to check if an expression can evaluate to a boolean
bool SemanticAnalyzer::isConditionExpr(Expr* expr) {
    if (!expr) return false;

    // If it's a binary expression
    if (auto* binary = dynamic_cast<BinaryExpr*>(expr)) {
        switch (binary->op.type) {
            // Comparison operators
            case LESS:
            case LESS_EQUAL:
            case GREATER:
            case GREATER_EQUAL:
            case EQUALS_EQUALS:
            case NOT_EQUALS:
                return true;
            
            // Logical operators
            case AND:
            case OR:
                return true;
            
            default:
                return false;
        }
    }
    
    // If it's a unary NOT operation
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        return unary->op.type == NOT;
    }

    // If it's a direct boolean value or expression
    auto exprType = getExprType(expr);
    return exprType && exprType->name == "bool";
}


bool SemanticAnalyzer::analyze(Program* program) {
    if (!program) return false;
    
    declareBuiltinFunctions();
    // Analyze the program
    program->accept(this);
    
    // Return true if no semantic errors were found
    return !ErrorHandler::instance().hasErrors(ErrorLevel::SEMANTIC);
}

void SemanticAnalyzer::declareBuiltinFunctions() {
    // Existing built-in functions
    std::vector<Parameter> printParams = {
        Parameter(Token(IDENTIFIER, "value", 0, 0), Type("any"))
    };
    symbolTable.declareFunction("print", Type("int"), printParams);
    
    std::vector<Parameter> inputParams = {
        Parameter(Token(IDENTIFIER, "prompt", 0, 0), Type("str"))
    };
    symbolTable.declareFunction("input", Type("str"), inputParams);
    
    // Add sizeof as a built-in function
    std::vector<Parameter> sizeofParams = {
        Parameter(Token(IDENTIFIER, "type", 0, 0), Type("any"))
    };
    symbolTable.declareFunction("sizeof", Type("int"), sizeofParams);
    
    // Memory management functions with correct signatures
    std::vector<Parameter> mallocParams = {
        Parameter(Token(IDENTIFIER, "size", 0, 0), Type("int"))
    };
    symbolTable.declareFunction("malloc", Type("any", true), mallocParams);
    
    std::vector<Parameter> freeParams = {
        Parameter(Token(IDENTIFIER, "ptr", 0, 0), Type("any", true))
    };
    symbolTable.declareFunction("free", Type("void"), freeParams);
    
    std::vector<Parameter> reallocParams = {
        Parameter(Token(IDENTIFIER, "ptr", 0, 0), Type("any", true)),
        Parameter(Token(IDENTIFIER, "size", 0, 0), Type("int"))
    };
    symbolTable.declareFunction("realloc", Type("any", true), reallocParams);
}

void SemanticAnalyzer::visit(Program* node) {
    mainFound = false;  // Reset mainFound flag
    
    // First pass: declare all functions (enables forward references)
    for (const auto& func : node->functions) {
        if (!symbolTable.declareFunction(func->name.value, func->returnType, func->parameters)) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                func->name.line,
                func->name.column,
                "Duplicate function declaration: " + func->name.value
            );
        }
        
        // Check if this is the main function
        if (func->name.value == "main") {
            mainFound = validateMainFunction(func.get());
        }
    }
    
    // Check if main was found after processing all functions
    if (!mainFound) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            0, 0,  // Could enhance this with actual file position
            "No valid main function found. Program must have a main function."
        );
    }
    
    // Second pass: analyze function bodies
    for (const auto& func : node->functions) {
        func->accept(this);
    }
}



void SemanticAnalyzer::visit(FunctionDecl* node) {
    // Check if this is the main function
    if (node->name.value == "main") {
        mainFound = validateMainFunction(node);
    }
    symbolTable.enterScope(); // Enter function scope
    currentFunctionReturnType = node->returnType;
    

    // Declare parameters in function scope
    for (const auto& param : node->parameters) {
        if (!symbolTable.declare(param.name.value, param.type)) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                param.name.line,
                param.name.column,
                "Duplicate parameter name: " + param.name.value
            );
        }
    }
    
    // Analyze function body
    node->body->accept(this);
    
    symbolTable.exitScope(); // Exit function scope
}

bool SemanticAnalyzer::validateMainFunction(FunctionDecl* func) {
    return isValidMainSignature(func);
}

bool SemanticAnalyzer::isValidMainSignature(FunctionDecl* func) {
    // First check the return type
    if (func->returnType.name != "int") {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            func->name.line,
            func->name.column,
            "Main function must return int, found: " + func->returnType.name
        );
        return false;
    }

    // Allow two forms: main() or main(argc: int, argv: str[])
    if (func->parameters.empty()) {
        return true;  // No-argument form is valid
    }
    
    // Check command-line arguments form
    if (func->parameters.size() == 2) {
        const auto& argc = func->parameters[0];
        const auto& argv = func->parameters[1];
        
        bool valid = true;
        
        // Check argc parameter
        if (argc.type.name != "int" || argc.type.isArray) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                argc.name.line,
                argc.name.column,
                "First parameter of main must be 'argc: int'"
            );
            valid = false;
        }
        
        // Check argv parameter
        if (argv.type.name != "str" || !argv.type.isArray) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                argv.name.line,
                argv.name.column,
                "Second parameter of main must be 'argv: str[]'"
            );
            valid = false;
        }
        
        return valid;
    }
    
    // If we get here, we have wrong number of parameters
    std::string foundParams;
    for (const auto& param : func->parameters) {
        if (!foundParams.empty()) foundParams += ", ";
        foundParams += param.type.name;
        if (param.type.isArray) foundParams += "[]";
        foundParams += " " + param.name.value;
    }
    
    ErrorHandler::instance().error(
        ErrorLevel::SEMANTIC,
        func->name.line,
        func->name.column,
        "Main function must either have no parameters or (argc: int, argv: str[]), found: (" + 
        foundParams + ")"
    );
    return false;
}

void SemanticAnalyzer::visit(VarDeclStmt* node) {
    // Check initializer type if present
    if (node->initializer) {
        node->initializer->accept(this);
        auto initType = getExprType(node->initializer.get());
        if (initType && !symbolTable.isCompatibleTypes(node->type, *initType)) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                node->name.line,
                node->name.column,
                "Type mismatch in variable declaration. Expected " + 
                node->type.name + " but got " + initType->name
            );
            return;
        }
    }
    
    // Declare the variable in the current scope
    if (!symbolTable.declare(node->name.value, node->type)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->name.line,
            node->name.column,
            "Variable already declared in this scope: " + node->name.value
        );
        return;
    }
}

void SemanticAnalyzer::visit(AssignExpr* node) {
    auto targetType = getExprType(node->target.get());
    auto valueType = getExprType(node->value.get());
    
    if (!targetType || !valueType) return;
    
    if (!symbolTable.isCompatibleTypes(*targetType, *valueType)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->op.line,
            node->op.column,
            "Type mismatch in assignment. Cannot assign " + 
            valueType->name + " to " + targetType->name
        );
    }
}

void SemanticAnalyzer::visit(BinaryExpr* node) {
    auto leftType = getExprType(node->left.get());
    auto rightType = getExprType(node->right.get());
    
    if (!leftType || !rightType) return;
    
    if (!checkBinaryOperatorTypes(node->op, *leftType, *rightType)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->op.line,
            node->op.column,
            "Invalid operand types for operator " + node->op.value
        );
    }
}

std::optional<Type> SemanticAnalyzer::getExprType(Expr* expr) {
    if (!expr) return std::nullopt;
    
    if (auto* num = dynamic_cast<NumberExpr*>(expr)) {
        return Type(num->isFloat ? "float" : "int");
    }
    else if (auto* str = dynamic_cast<StringExpr*>(expr)) {
        return Type("str");
    }
    else if (auto* boolean = dynamic_cast<BoolExpr*>(expr)) {
        return Type("bool");
    }
    else if (auto* var = dynamic_cast<VariableExpr*>(expr)) {
        if (auto* symbol = symbolTable.resolve(var->name.value)) {
            return symbol->type;
        }
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            var->name.line,
            var->name.column,
            "Undefined variable: " + var->name.value
        );
    }
    // Add other expression types as needed
    
    return std::nullopt;
}

bool SemanticAnalyzer::checkBinaryOperatorTypes(const Token& op, const Type& left, const Type& right) {
    // Arithmetic operators
    if (op.type == PLUS || op.type == MINUS || op.type == STAR || op.type == SLASH) {
        return (left.name == "int" || left.name == "float") &&
               (right.name == "int" || right.name == "float");
    }
    
    // Comparison operators
    if (op.type == LESS || op.type == LESS_EQUAL || 
        op.type == GREATER || op.type == GREATER_EQUAL) {
        return (left.name == "int" || left.name == "float") &&
               (right.name == "int" || right.name == "float");
    }
    
    // Equality operators
    if (op.type == EQUALS_EQUALS || op.type == NOT_EQUALS) {
        return symbolTable.isCompatibleTypes(left, right);
    }
    
    // Logical operators
    if (op.type == AND || op.type == OR) {
        return left.name == "bool" && right.name == "bool";
    }
    
    return false;
}

void SemanticAnalyzer::visit(BlockStmt* node) {
    // Don't create a new scope for function bodies as they already have one
    bool isNewScope = true;
    if (auto* funcDecl = dynamic_cast<FunctionDecl*>(node->parent)) {
        if (funcDecl->body.get() == node) {
            isNewScope = false;
        }
    }
    
    if (isNewScope) {
        symbolTable.enterScope();
    }
    
    for (const auto& stmt : node->statements) {
        stmt->accept(this);
    }
    
    if (isNewScope) {
        symbolTable.exitScope();
    }
}

void SemanticAnalyzer::visit(IfStmt* node) {
    node->condition->accept(this);
    
    if (!isConditionExpr(node->condition.get())) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->condition->loc.line,
            node->condition->loc.column,
            "If condition must evaluate to a boolean value"
        );
    }

    node->thenBranch->accept(this);
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }
}

void SemanticAnalyzer::visit(WhileStmt* node) {
    node->condition->accept(this);
    
    if (!isConditionExpr(node->condition.get())) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->condition->loc.line,
            node->condition->loc.column,
            "While condition must evaluate to a boolean value"
        );
    }

    node->body->accept(this);
}

void SemanticAnalyzer::visit(ReturnStmt* node) {
    if (!node->value) {
        if (currentFunctionReturnType.name != "void") {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                node->keyword.line,
                node->keyword.column,
                "Function must return a value of type " + currentFunctionReturnType.name
            );
        }
        return;
    }

    auto returnType = getExprType(node->value.get());
    if (returnType && !symbolTable.isCompatibleTypes(currentFunctionReturnType, *returnType)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->value->loc.line,
            node->value->loc.column,
            "Return type mismatch. Expected " + currentFunctionReturnType.name + 
            " but got " + returnType->name
        );
    }
}

void SemanticAnalyzer::visit(NumberExpr* node) {
    // Nothing to check - type is inherent
}

void SemanticAnalyzer::visit(StringExpr* node) {
    // Nothing to check - type is inherent
}

void SemanticAnalyzer::visit(BoolExpr* node) {
    // Nothing to check - type is inherent
}

void SemanticAnalyzer::visit(VariableExpr* node) {
    if (!symbolTable.resolve(node->name.value)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->name.line,
            node->name.column,
            "Undefined variable: " + node->name.value
        );
    }
}

void SemanticAnalyzer::visit(ArrayAccessExpr* node) {
    auto arrayType = getExprType(node->array.get());
    auto indexType = getExprType(node->index.get());

    if (!arrayType) return;

    // Check if the type is actually an array
    if (!arrayType->isArray) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->array->loc.line,
            node->array->loc.column,
            "Cannot index non-array type"
        );
        return;
    }

    // Check if index is an integer
    if (indexType && indexType->name != "int") {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->index->loc.line,
            node->index->loc.column,
            "Array index must be an integer"
        );
    }
}

void SemanticAnalyzer::visit(UnaryExpr* node) {
    auto operandType = getExprType(node->expr.get());
    if (!operandType) return;

    switch (node->op.type) {
        case MINUS:
            if (operandType->name != "int" && operandType->name != "float") {
                ErrorHandler::instance().error(
                    ErrorLevel::SEMANTIC,
                    node->op.line,
                    node->op.column,
                    "Unary minus requires numeric operand"
                );
            }
            break;
        case NOT:
            if (operandType->name != "bool") {
                ErrorHandler::instance().error(
                    ErrorLevel::SEMANTIC,
                    node->op.line,
                    node->op.column,
                    "Logical not requires boolean operand"
                );
            }
            break;
        default:
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                node->op.line,
                node->op.column,
                "Unknown unary operator"
            );
    }
}

void SemanticAnalyzer::visit(CallExpr* node) {
    auto* func = symbolTable.resolveFunction(node->name.value);
    if (!func) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->name.line,
            node->name.column,
            "Undefined function: " + node->name.value
        );
        return;
    }

    // Check argument count
    if (func->parameters.size() != node->arguments.size()) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->name.line,
            node->name.column,
            "Wrong number of arguments to function " + node->name.value +
            ". Expected " + std::to_string(func->parameters.size()) +
            " but got " + std::to_string(node->arguments.size())
        );
        return;
    }

    // Check argument types
    for (size_t i = 0; i < node->arguments.size(); i++) {
        auto argType = getExprType(node->arguments[i].get());
        if (argType && !symbolTable.isCompatibleTypes(func->parameters[i].type, *argType)) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                node->arguments[i]->loc.line,
                node->arguments[i]->loc.column,
                "Argument type mismatch. Expected " + func->parameters[i].type.name +
                " but got " + argType->name
            );
        }
    }
}

void SemanticAnalyzer::visit(ArrayInitExpr* node) {
    if (node->elements.empty()) return;

    // Get the type of the first element
    auto firstType = getExprType(node->elements[0].get());
    if (!firstType) return;

    // Check that all elements have compatible types
    for (size_t i = 1; i < node->elements.size(); i++) {
        auto elemType = getExprType(node->elements[i].get());
        if (elemType && !symbolTable.isCompatibleTypes(*firstType, *elemType)) {
            ErrorHandler::instance().error(
                ErrorLevel::SEMANTIC,
                node->elements[i]->loc.line,
                node->elements[i]->loc.column,
                "Array elements must have compatible types"
            );
        }
    }
}

void SemanticAnalyzer::visit(ArrayAllocExpr* node) {
    auto sizeType = getExprType(node->size.get());
    if (sizeType && sizeType->name != "int") {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            node->size->loc.line,
            node->size->loc.column,
            "Array size must be an integer"
        );
    }
}

void SemanticAnalyzer::visit(ExprStmt* node) {
    node->expr->accept(this);
}

void SemanticAnalyzer::visit(TypeExpr* node) {
    // Nothing to check - type is inherent
}

