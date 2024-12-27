#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "ast.h"
#include "error_handler.h"
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>

// Base symbol class that handles all compilation stages
class Symbol {
public:
    enum class Kind {
        VARIABLE,
        FUNCTION
    };

    Symbol(const std::string& name, const Type& type, Kind kind)
        : name(name), type(type), kind(kind), llvmValue(nullptr) {}

    virtual ~Symbol() = default;

    std::string name;
    Type type;
    Kind kind;
    llvm::Value* llvmValue;     // For variables: alloca or global value
    bool isAlloca = false;      // Track if the value is an alloca instruction
};

// Function-specific symbol information
class FunctionSymbol : public Symbol {
public:
    FunctionSymbol(const std::string& name, const Type& returnType,
                  const std::vector<Parameter>& params)
        : Symbol(name, returnType, Kind::FUNCTION), 
          parameters(params), llvmFunction(nullptr) {}

    std::vector<Parameter> parameters;
    llvm::Function* llvmFunction;  // LLVM Function representation
};

// Represents a single scope level
class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent(parent) {}

    bool declare(const std::string& name, const Type& type);
    bool declareFunction(const std::string& name, const Type& returnType,
                        const std::vector<Parameter>& params);
    Symbol* resolve(const std::string& name);

    
    const std::unordered_map<std::string, std::unique_ptr<Symbol>>& getSymbols() const {
        return symbols;
    }
    
    Scope* getParent() const { return parent; }

private:
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    Scope* parent;
};

// Main symbol table class as a singleton
class SymbolTable {
public:
    // Delete copy constructor and assignment operator
    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;

    // Singleton access method
    static SymbolTable& instance() {
        static SymbolTable instance;
        return instance;
    }

    // Scope management
    void enterScope() { scopes.push_back(std::make_unique<Scope>(currentScope())); }
    void exitScope() { if (!scopes.empty()) scopes.pop_back(); }
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

    // Access to scopes for iteration if needed
    const std::vector<std::unique_ptr<Scope>>& getScopes() const { return scopes; }
    void print() const;

private:
    SymbolTable() { enterScope(); }  // Create initial global scope
    std::vector<std::unique_ptr<Scope>> scopes;
};

#endif // SYMBOL_TABLE_H