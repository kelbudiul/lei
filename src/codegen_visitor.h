#ifndef CODEGEN_VISITOR_H
#define CODEGEN_VISITOR_H

#include "visitor.h"
#include "ast.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

class CodeGenVisitor : public Visitor {
private:
    llvm::LLVMContext& context;
    llvm::IRBuilder<>& builder;
    llvm::Module& module;
    
    std::map<std::string, llvm::AllocaInst*> namedValues;
    llvm::Function* currentFunction;
    
    llvm::Value* lastValue;
    
    llvm::Type* getLLVMType(Type type);
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function,
                                            const std::string& name,
                                            llvm::Type* type);

public:
    CodeGenVisitor(llvm::LLVMContext& ctx, 
                   llvm::IRBuilder<>& bldr,
                   llvm::Module& mod);
    
    void visit(FunctionAST* node) override;
    void visit(BlockAST* node) override;
    void visit(IfStatementAST* node) override;
    void visit(WhileStatementAST* node) override;
    void visit(ReturnAST* node) override;
    void visit(VariableDeclarationAST* node) override;
    void visit(BinaryExprAST* node) override;
    void visit(UnaryExprAST* node) override;
    void visit(LiteralAST* node) override;
    void visit(VariableExprAST* node) override;
    void visit(CallExprAST* node) override;
    void visit(AssignmentExprAST* node) override;
    
    llvm::Value* getLastValue() const { return lastValue; }
};

#endif // CODEGEN_VISITOR_H
