#include "symbol_table.h"
#include <iostream> // For std::cout

void SymbolTable::print() const {
    std::cout << "Symbol Table Contents:\n";

    int scopeLevel = 0;
    for (const auto& scope : scopes) {
        std::cout << "Scope Level " << scopeLevel++ << ":\n";

        for (const auto& [name, symbol] : scope->getSymbols()) {
            std::cout << "  Name: " << name
                      << ", Type: " << symbol->type.name
                      << ", Kind: " << (symbol->kind == Symbol::Kind::VARIABLE ? "Variable" : "Function");

            if (symbol->kind == Symbol::Kind::FUNCTION) {
                auto* funcSymbol = static_cast<FunctionSymbol*>(symbol.get());
                std::cout << ", Return Type: " << funcSymbol->type.name
                          << ", Parameters: ";
                for (const auto& param : funcSymbol->parameters) {
                    std::cout << param.name.value << ": " << param.type.name << ", ";
                }
                std::cout << "LLVM Function: " << (funcSymbol->llvmFunction ? "Assigned" : "Null");
            }

            if (symbol->llvmValue) {
                std::cout << ", LLVM Value: Assigned";
            } else {
                std::cout << ", LLVM Value: Null";
            }

            std::cout << '\n';
        }
    }
    std::cout << "---- End of Symbol Table ----\n";
}



bool Scope::declare(const std::string& name, const Type& type) {
    // Check if symbol already exists in current scope
    if (symbols.find(name) != symbols.end()) {
        return false;
    }

    symbols[name] = std::make_unique<Symbol>(name, type, Symbol::Kind::VARIABLE);
    return true;
}

bool Scope::declareFunction(const std::string& name, const Type& returnType,
                          const std::vector<Parameter>& params) {
    // Check if function already exists in current scope
    if (symbols.find(name) != symbols.end()) {
        return false;
    }

    symbols[name] = std::make_unique<FunctionSymbol>(name, returnType, params);
    return true;
}

Symbol* Scope::resolve(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second.get();
    }
    
    // If not found in current scope and we have a parent, check there
    return parent ? parent->resolve(name) : nullptr;
}

bool SymbolTable::declare(const std::string& name, const Type& type) {
    if (!currentScope()) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            0, 0,  // TODO: Add proper location info
            "No active scope for declaration"
        );
        return false;
    }

    if (!currentScope()->declare(name, type)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            0, 0,  // TODO: Add proper location info
            "Symbol '" + name + "' already declared in current scope"
        );
        return false;
    }

    return true;
}

bool SymbolTable::declareFunction(const std::string& name, const Type& returnType,
                                const std::vector<Parameter>& params) {
    if (!currentScope()) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            0, 0,
            "No active scope for function declaration"
        );
        return false;
    }

    if (!currentScope()->declareFunction(name, returnType, params)) {
        ErrorHandler::instance().error(
            ErrorLevel::SEMANTIC,
            0, 0,
            "Function '" + name + "' already declared in current scope"
        );
        return false;
    }

    return true;
}

Symbol* SymbolTable::resolve(const std::string& name) {
    if (!currentScope()) return nullptr;
    return currentScope()->resolve(name);
}

FunctionSymbol* SymbolTable::resolveFunction(const std::string& name) {
    Symbol* symbol = resolve(name);
    if (!symbol || symbol->kind != Symbol::Kind::FUNCTION) {
        return nullptr;
    }
    return static_cast<FunctionSymbol*>(symbol);
}

bool SymbolTable::isCompatibleTypes(const Type& left, const Type& right) const {
    // Special case: 'any' type is compatible with everything
    if (left.name == "any" || right.name == "any") {
        return true;
    }

    // If types are exactly the same
    if (left.name == right.name && left.isArray == right.isArray) {
        // For arrays, check sizes if both are fixed-size
        if (left.isArray && right.isArray) {
            return left.arraySize == -1 || right.arraySize == -1 || 
                   left.arraySize == right.arraySize;
        }
        return true;
    }

    // Handle numeric type conversions (int to float is allowed)
    if (!left.isArray && !right.isArray) {
        if (left.name == "float" && right.name == "int") {
            return true;
        }
    }

    return false;
}

Type SymbolTable::getCommonType(const Type& left, const Type& right) const {
    // If types are exactly the same, return either
    if (left.name == right.name && left.isArray == right.isArray) {
        // For arrays, use the more specific size
        if (left.isArray) {
            int size = (left.arraySize != -1) ? left.arraySize : right.arraySize;
            return Type(left.name, true, size);
        }
        return left;
    }

    // Handle numeric type conversions
    if (!left.isArray && !right.isArray) {
        if (left.name == "float" || right.name == "float") {
            return Type("float");
        }
    }

    // Default to left type if no common type found
    return left;
}
