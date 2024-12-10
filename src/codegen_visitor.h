#ifndef CODEGEN_VISITOR_H
#define CODEGEN_VISITOR_H

#include "../include/common.h"
#include "ast.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"


class CodeGenerationVisitor : public Visitor {
private:
    llvm::LLVMContext& context;
    llvm::IRBuilder<>& builder;
    llvm::Module& module;

public:
    CodeGenerationVisitor(llvm::LLVMContext& ctx, 
                          llvm::IRBuilder<>& bldr, 
                          llvm::Module& mod);

    void visit(ASTNode* node) override;
    void visit(FunctionAST* node) override;
    void visit(ReturnAST* node) override;
    void visit(BinaryExpressionAST* node) override;
    void visit(ExpressionAST* node) override;



    llvm::Module& getModule(); 


};

#endif // CODEGEN_VISITOR_H