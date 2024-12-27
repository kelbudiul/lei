#ifndef COMPILER_H
#define COMPILER_H

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LLVMContext.h>
#include <memory>
#include <string>
#include "symbol_table.h"
#include "error_handler.h"
#include "ast.h"
#include <iostream>

class Compiler {
public:
    llvm::LLVMContext llvmContext;
    ErrorHandler& errorHandler = ErrorHandler::instance();
    SymbolTable& symbolTable = SymbolTable::instance();  // Use singleton instance instead of direct member

    bool compile(const std::string& source, const std::string& outputPath);
    bool execute(const std::string& source);
};

#endif // COMPILER_H