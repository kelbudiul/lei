#include "codegen_visitor.h"

CodeGenerationVisitor::CodeGenerationVisitor(
    llvm::LLVMContext& ctx,
    llvm::IRBuilder<>& bldr,
    llvm::Module& mod
) : context(ctx), builder(bldr), module(mod) {}


void CodeGenerationVisitor::visit(BinaryExpressionAST* node) {

    node->left->accept(this);
    llvm::Value* L = valueStack.top(); valueStack.pop();
    
    node->right->accept(this);
    llvm::Value* R = valueStack.top(); valueStack.pop();

    llvm::Value* result;
    if (node->op == "+")
        result = builder.CreateAdd(L, R, "addtmp");
    else if (node->op == "-")
        result = builder.CreateSub(L, R, "subtmp");
    else if (node->op == "*")
        result = builder.CreateMul(L, R, "multmp");
    else if (node->op == "/")
        result = builder.CreateSDiv(L, R, "divtmp");
    else
        throw std::runtime_error("Unknown binary operator");

    valueStack.push(result);
}
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

void CodeGenVisitor::visit(VariableDeclarationAST* node) {
    llvm::Type* varType = getLLVMType(stringToType(node->type));
    
    // Create allocation for the variable
    llvm::AllocaInst* alloca = createEntryBlockAlloca(
        currentFunction, node->name, varType);
    
    // Store the allocation
    namedValues[node->name] = alloca;

    // If there's an initializer, generate code for it
    if (node->initializer) {
        node->initializer->accept(this);
        llvm::Value* initValue = lastValue;
        builder.CreateStore(initValue, alloca);
    } else {
        // Initialize with default value based on type
        llvm::Value* defaultValue;
        Type type = stringToType(node->type);
        
        switch (type) {
            case Type::Int:
                defaultValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
                break;
            case Type::Float:
                defaultValue = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
                break;
            case Type::Bool:
                defaultValue = llvm::ConstantInt::get(context, llvm::APInt(1, 0));
                break;
            case Type::String:
                // Create empty string constant
                defaultValue = builder.CreateGlobalStringPtr("");
                break;
            default:
                throw std::runtime_error("Unsupported type for default initialization");
        }
        
        builder.CreateStore(defaultValue, alloca);
    }
    
    lastValue = alloca;
}


// Optional: Add getters for the generated LLVM module
llvm::Module& CodeGenerationVisitor::getModule() {
    return module;
}