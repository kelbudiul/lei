#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "ast.h"
#include "error_handler.h"


// Forward declarations
class SymbolTable;

// Represents a single symbol (variable or function)
class Symbol {
public:
    enum class Kind {
        VARIABLE,
        FUNCTION
    };

    Symbol(const std::string& name, const Type& type, Kind kind)
        : name(name), type(type), kind(kind) {}

    std::string name;
    Type type;
    Kind kind;
};

// Function-specific symbol information
class FunctionSymbol : public Symbol {
public:
    FunctionSymbol(const std::string& name, const Type& returnType,
                  const std::vector<Parameter>& params)
        : Symbol(name, returnType, Kind::FUNCTION), parameters(params) {}

    std::vector<Parameter> parameters;
};

// Represents a single scope level
class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent(parent) {}

    // Symbol management
    bool declare(const std::string& name, const Type& type);
    bool declareFunction(const std::string& name, const Type& returnType,
                        const std::vector<Parameter>& params);
    Symbol* resolve(const std::string& name);
    
    // Parent scope access
    Scope* getParent() const { return parent; }

private:
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    Scope* parent;
};

// Main symbol table class
class SymbolTable {
public:
    SymbolTable() { enterScope(); }  // Create global scope

    // Scope management
    void enterScope() { scopes.push_back(std::make_unique<Scope>(currentScope())); }
    void exitScope() { 
        if (!scopes.empty()) scopes.pop_back(); 
    }
    Scope* currentScope() const { 
        return scopes.empty() ? nullptr : scopes.back().get(); 
    }

    // Symbol management
    bool declare(const std::string& name, const Type& type);
    bool declareFunction(const std::string& name, const Type& returnType,
                        const std::vector<Parameter>& params);
    Symbol* resolve(const std::string& name);
    FunctionSymbol* resolveFunction(const std::string& name);

    // Type checking helpers
    bool isCompatibleTypes(const Type& left, const Type& right) const;
    Type getCommonType(const Type& left, const Type& right) const;

private:
    std::vector<std::unique_ptr<Scope>> scopes;
};

#endif // SYMBOL_TABLE_H