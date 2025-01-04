#ifndef CODEGEN_VISITOR_H
#define CODEGEN_VISITOR_H

#include "visitor.h"
#include "ast.h"
#include "symbol_table.h"
#include "error_handler.h"
#include "type_helper.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/BasicBlock.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>
#include <unordered_set>


class CodegenVisitor : public Visitor {
public:
    CodegenVisitor(llvm::LLVMContext& ctx);
    ~CodegenVisitor() = default;

    // Main entry point for code generation
    std::unique_ptr<llvm::Module> generateModule(Program* program, const std::string& moduleName);
    

    // AST Visitor interface implementation
    void visit(Program* node) override;
    void visit(FunctionDecl* node) override;
    void visit(NumberExpr* node) override;
    void visit(StringExpr* node) override;
    void visit(BoolExpr* node) override;
    void visit(VariableExpr* node) override;
    void visit(ArrayAccessExpr* node) override;
    void visit(BinaryExpr* node) override;
    void visit(UnaryExpr* node) override;
    void visit(AssignExpr* node) override;
    void visit(CallExpr* node) override;
    void visit(ArrayInitExpr* node) override;
    void visit(ArrayAllocExpr* node) override;
    void visit(ExprStmt* node) override;
    void visit(VarDeclStmt* node) override;
    void visit(BlockStmt* node) override;
    void visit(IfStmt* node) override;
    void visit(WhileStmt* node) override;
    void visit(ReturnStmt* node) override;
    void visit(TypeExpr* node) override;

private:
    llvm::LLVMContext& context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    llvm::Function* currentFunction;
    llvm::Value* lastValue;
    std::unordered_map<std::string, llvm::Value*> stringConstants;
    SymbolTable& symbolTable = SymbolTable::instance(); // Reference to singleton symbol table
    ErrorHandler& errorHandler = ErrorHandler::instance();  // Reference to singleton error handler
    TypeHelper& typeHelper;
    bool isAssignmentTarget = false;

    // Helper methods for type conversion and code generation
    ASTNode* getCurrentParent(ASTNode* node);
    llvm::Value* generateAlloca(llvm::Function* function, const std::string& name, llvm::Type* type);
    void declareRuntimeFunctions();
    void declareFunction(const std::string& name, llvm::Type* returnType, 
                                     const std::vector<llvm::Type*>& paramTypes, bool isVarArgs = false);
                                      llvm::Value* handleBuiltinFunction(CallExpr* node);
    llvm::Value* handleRegularFunctionCall(CallExpr* node);
    std::vector<llvm::Value*> processCallArguments(CallExpr* node, FunctionSymbol* funcSymbol);

    llvm::Value* createVariableAllocation(VarDeclStmt* node);
    void handleVariableInitialization(VarDeclStmt* node, llvm::Value* alloca);
    void handleArrayInitialization(VarDeclStmt* node, llvm::Value* alloca, ArrayInitExpr* arrayInit);
    llvm::Value* handleArrayArgument(llvm::Value* arg);
    void initializeFixedArray(VarDeclStmt* node, llvm::Value* alloca, ArrayInitExpr* arrayInit);
    void initializeDynamicArray(VarDeclStmt* node, llvm::Value* alloca);
    void updateSymbolTableEntry(const std::string& name, const Type& type, llvm::Value* alloca);

    // Built-in function generators
    void generatePrintCall(CallExpr* node);
    void generateInputCall(CallExpr* node);
    void generateMallocCall(CallExpr* node);
    void generateFreeCall(CallExpr* node);
    void generateReallocCall(CallExpr* node);
    void generateStrlenCall(CallExpr* node);
    llvm::Value* generateSizeofCall(CallExpr* node);

    void reportError(const std::string& message, const Location& loc);
};

#endif // CODEGEN_VISITOR_H