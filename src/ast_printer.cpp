#include "ast_printer.h"


std::string ASTPrinter::print(ASTNode* node) {
    output.str("");  // Clear the stringstream
    indent = 0;
    node->accept(this);
    return output.str();
}

void ASTPrinter::writeLine(const std::string& text) {
    output << std::string(indent * 2, ' ') << text << '\n';
}

std::string ASTPrinter::formatType(const Type& type) {
    std::string result = type.name;
    if (type.isArray) {
        result += "[";
        if (type.arraySize >= 0) {
            result += std::to_string(type.arraySize);
        }
        result += "]";
    }
    return result;
}


void ASTPrinter::visit(TypeExpr* node) {
    writeLine("Type: " + formatType(node->type));
}


void ASTPrinter::visit(Program* node) {
    writeLine("Program");
    indent++;
    for (const auto& function : node->functions) {
        function->accept(this);
    }
    indent--;
}

void ASTPrinter::visit(FunctionDecl* node) {
    writeLine("Function: " + node->name.value);
    indent++;
    writeLine("Return Type: " + formatType(node->returnType));
    
    if (!node->parameters.empty()) {
        writeLine("Parameters:");
        indent++;
        for (const auto& param : node->parameters) {
            writeLine(param.name.value + ": " + formatType(param.type));
        }
        indent--;
    }
    
    writeLine("Body:");
    indent++;
    node->body->accept(this);
    indent--;
    indent--;
}

void ASTPrinter::visit(NumberExpr* node) {
    writeLine("Number: " + node->token.value + 
             (node->isFloat ? " (float)" : " (int)"));
}

void ASTPrinter::visit(StringExpr* node) {
    writeLine("String: \"" + node->token.value + "\"");
}

void ASTPrinter::visit(BoolExpr* node) {
    writeLine("Boolean: " + std::string(node->value ? "true" : "false"));
}

void ASTPrinter::visit(VariableExpr* node) {
    writeLine("Variable: " + node->name.value);
}

void ASTPrinter::visit(ArrayAccessExpr* node) {
    writeLine("Array Access:");
    indent++;
    writeLine("Array:");
    indent++;
    node->array->accept(this);
    indent--;
    writeLine("Index:");
    indent++;
    node->index->accept(this);
    indent--;
    indent--;
}

void ASTPrinter::visit(BinaryExpr* node) {
    writeLine("Binary Expression: " + node->op.value);
    indent++;
    writeLine("Left:");
    indent++;
    node->left->accept(this);
    indent--;
    writeLine("Right:");
    indent++;
    node->right->accept(this);
    indent--;
    indent--;
}

void ASTPrinter::visit(UnaryExpr* node) {
    writeLine("Unary Expression: " + node->op.value);
    indent++;
    node->expr->accept(this);
    indent--;
}

void ASTPrinter::visit(AssignExpr* node) {
    writeLine("Assignment: " + node->op.value);
    indent++;
    writeLine("Target:");
    indent++;
    node->target->accept(this);
    indent--;
    writeLine("Value:");
    indent++;
    node->value->accept(this);
    indent--;
    indent--;
}

void ASTPrinter::visit(CallExpr* node) {
    writeLine("Function Call: " + node->name.value);
    if (!node->arguments.empty()) {
        indent++;
        writeLine("Arguments:");
        indent++;
        for (const auto& arg : node->arguments) {
            arg->accept(this);
        }
        indent--;
        indent--;
    }
}

void ASTPrinter::visit(ArrayInitExpr* node) {
    writeLine("Array Initializer:");
    if (!node->elements.empty()) {
        indent++;
        writeLine("Elements:");
        indent++;
        for (const auto& elem : node->elements) {
            elem->accept(this);
        }
        indent--;
        indent--;
    }
}

void ASTPrinter::visit(ArrayAllocExpr* node) {
    writeLine("Array Allocation: " + formatType(node->elementType));
    indent++;
    writeLine("Size:");
    indent++;
    node->size->accept(this);
    indent--;
    indent--;
}

void ASTPrinter::visit(ExprStmt* node) {
    writeLine("Expression Statement:");
    indent++;
    node->expr->accept(this);
    indent--;
}

void ASTPrinter::visit(VarDeclStmt* node) {
    writeLine("Variable Declaration: " + node->name.value);
    indent++;
    writeLine("Type: " + formatType(node->type));
    if (node->initializer) {
        writeLine("Initializer:");
        indent++;
        node->initializer->accept(this);
        indent--;
    }
    indent--;
}

void ASTPrinter::visit(BlockStmt* node) {
    writeLine("Block:");
    indent++;
    for (const auto& stmt : node->statements) {
        stmt->accept(this);
    }
    indent--;
}

void ASTPrinter::visit(IfStmt* node) {
    writeLine("If Statement:");
    indent++;
    writeLine("Condition:");
    indent++;
    node->condition->accept(this);
    indent--;
    writeLine("Then:");
    indent++;
    node->thenBranch->accept(this);
    indent--;
    if (node->elseBranch) {
        writeLine("Else:");
        indent++;
        node->elseBranch->accept(this);
        indent--;
    }
    indent--;
}

void ASTPrinter::visit(WhileStmt* node) {
    writeLine("While Statement:");
    indent++;
    writeLine("Condition:");
    indent++;
    node->condition->accept(this);
    indent--;
    writeLine("Body:");
    indent++;
    node->body->accept(this);
    indent--;
    indent--;
}

void ASTPrinter::visit(ReturnStmt* node) {
    writeLine("Return Statement");
    if (node->value) {
        indent++;
        node->value->accept(this);
        indent--;
    }
}
