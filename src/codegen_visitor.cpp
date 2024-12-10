#include "codegen_visitor.h"

CodeGenerationVisitor::CodeGenerationVisitor(
    llvm::LLVMContext& ctx,
    llvm::IRBuilder<>& bldr,
    llvm::Module& mod
) : context(ctx), builder(bldr), module(mod) {}

void CodeGenerationVisitor::visit(ASTNode* node) {
    // Base case - should not be called directly
    throw std::runtime_error("Cannot generate code for base ASTNode");
}

void CodeGenerationVisitor::visit(FunctionAST* node) {
    // Create function type
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        builder.getInt32Ty(), // Return type
        false                 // No parameters
    );

    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        node->name,
        module
    );

    // Create a basic block to start insertion into
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(bb);

    // Generate code for the function body
    node->body->accept(this);
}

void CodeGenerationVisitor::visit(ReturnAST* node) {
    if (std::holds_alternative<int>(node->value)) {
        // Create return instruction
        int returnValue = std::get<int>(node->value);
        builder.CreateRet(builder.getInt32(returnValue));
    } else if (std::holds_alternative<float>(node->value)) {
        float returnValue = std::get<float>(node->value);
        builder.CreateRet(builder.getInt32(static_cast<int>(returnValue)));
    } else {
        throw std::runtime_error("Unsupported return type");
    }
}

// Optional: Add getters for the generated LLVM module
llvm::Module& CodeGenerationVisitor::getModule() {
    return module;
}