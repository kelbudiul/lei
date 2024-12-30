#ifndef CODEGEN_VISITOR_H
#define CODEGEN_VISITOR_H

#include "visitor.h"
#include "ast.h"
#include "symbol_table.h"
#include "error_handler.h"
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
    bool isAssignmentTarget = false;

    // Helper methods for type conversion and code generation
    ASTNode* getCurrentParent(ASTNode* node);
    llvm::Type* getLLVMType(const Type& type);
    llvm::Value* generateAlloca(llvm::Function* function, const std::string& name, llvm::Type* type);
    void createBasicBlocksForLoop(llvm::Function* function, const std::string& loopName,
                                llvm::BasicBlock*& condBlock,
                                llvm::BasicBlock*& bodyBlock,
                                llvm::BasicBlock*& endBlock);
    void declareRuntimeFunctions();
    llvm::Function* getIntrinsicFunction(const std::string& name);
    llvm::Value* handleArrayArgument(llvm::Value* arrayValue);
    void reportError(const std::string& message, const Location& loc);
    llvm::Value* convertValue(llvm::Value* value, llvm::Type* targetType);
    void handleFixedSizeArrayDecl(VarDeclStmt* node, llvm::Type* arrayType);
    void handleDynamicArrayDecl(VarDeclStmt* node, llvm::Type* pointerType);
    // Built-in function generators
    void generatePrintCall(CallExpr* node);
    void generateInputCall(CallExpr* node);
    void generateMallocCall(CallExpr* node);
    void generateFreeCall(CallExpr* node);
    void generateReallocCall(CallExpr* node);
    void generateStrlenCall(CallExpr* node);
    llvm::Value* generateSizeofCall(CallExpr* node);
};

#endif // CODEGEN_VISITOR_H