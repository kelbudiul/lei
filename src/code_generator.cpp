#include "code_generator.h"

void CodeGenerator::generateAndRun(std::unique_ptr<FunctionAST> &functionAST) {
    // LLVM initialization
    auto jit = llvm::orc::LLJITBuilder().create();
    if (!jit) {
        llvm::errs() << "Error creating JIT: " << jit.takeError() << "\n";
        return;
    }

    auto context = std::make_unique<llvm::LLVMContext>();
    llvm::IRBuilder<> builder(*context);
    
    auto module = std::make_unique<llvm::Module>("my_module", *context);

    // Create the function prototype
    llvm::FunctionType *funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    llvm::Function *func = llvm::Function::Create(funcType, 
        llvm::Function::ExternalLinkage, functionAST->name, *module);

    // Create the function body
    llvm::BasicBlock *block = llvm::BasicBlock::Create(*context, "entry", func);
    builder.SetInsertPoint(block);

    // Extract integer value safely
    int value = std::get<int>(dynamic_cast<ReturnAST *>(functionAST->body.get())->value);
    builder.CreateRet(builder.getInt32(value));

    // Verify the function
    llvm::verifyFunction(*func);

    auto &jitEngine = *jit.get();

    // Add the module to the JIT
    if (auto err = jitEngine.addIRModule(
        llvm::orc::ThreadSafeModule(std::move(module), 
        std::make_unique<llvm::LLVMContext>()))) {
        llvm::errs() << "Error adding module to JIT: " << err << "\n";
        return;
    }

    // Look up the function in the JIT
    auto mainSym = jitEngine.lookup(functionAST->name);
    if (!mainSym) {
        llvm::errs() << "Error finding function: " << mainSym.takeError() << "\n";
        return;
    }

    // Execute the function
    using FuncType = int (*)();
    auto *mainFunc = reinterpret_cast<FuncType>(mainSym->getAddress());
    
    int result = mainFunc();
    std::cout << "Function '" << functionAST->name << "' returned: " << result << std::endl;
}