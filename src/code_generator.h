#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "../include/common.h"
#include "ast.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"

// LLVM Code generation class
class CodeGenerator {
public:
    // Generate and run LLVM IR for a function
    static void generateAndRun(std::unique_ptr<FunctionAST> &functionAST);
};

#endif // CODE_GENERATOR_H