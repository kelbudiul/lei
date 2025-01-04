#ifndef TYPE_HELPER_H
#define TYPE_HELPER_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include "ast.h"
#include "error_handler.h"

// TypeHelper handles all type-related operations in code generation
class TypeHelper {
public:
     
    static TypeHelper& instance(llvm::LLVMContext* context = nullptr, 
                                llvm::IRBuilder<>* builder = nullptr) {
        static TypeHelper instance;
        if (context && builder && !instance.isInitialized()) {
            instance.context = context;
            instance.builder = builder;
            instance.initialized = true;
        }
        if (!instance.isInitialized()) {
            // We could throw an exception or use the error handler here
            ErrorHandler::instance().error(
                ErrorLevel::CODEGEN,
                0, 0,
                "TypeHelper used before initialization"
            );
        }
        return instance;
    }

    // Core type operations
    llvm::Value* convert(llvm::Value* value, llvm::Type* targetType);
    llvm::Type* getLLVMType(const Type& type);
    bool areTypesCompatible(llvm::Type* source, llvm::Type* target);

    // Type promotion for operations
    struct OperandPair {
        llvm::Value* left;
        llvm::Value* right;
        llvm::Type* commonType;
    };
    OperandPair promoteOperands(llvm::Value* left, llvm::Value* right, const Location& loc);

    // Array type handling
    llvm::Value* getArrayPointer(llvm::Value* array, const Location& loc);
    llvm::Type* getArrayElementType(llvm::Type* arrayType);
    
    // Type checking utilities
    bool isNumericType(llvm::Type* type) const;
    bool isArrayType(llvm::Type* type) const;
    bool isPointerType(llvm::Type* type) const;
    llvm::Value* getArrayElementPtr(llvm::Value* array, llvm::Value* index);

private:
     TypeHelper() : context(nullptr), builder(nullptr), initialized(false) {}
    bool isInitialized() const { return initialized; }
    bool isArrayOrPointerType(llvm::Type* type) const;
    llvm::Value* convertToFloatIfNeeded(llvm::Value* value);
    llvm::Value* convertToIntN(llvm::Value* value, unsigned width);
    llvm::LLVMContext* context;
    llvm::IRBuilder<>* builder;
    bool initialized;
    ErrorHandler& errorHandler = ErrorHandler::instance();

    // Internal conversion helpers
    llvm::Value* convertNumeric(llvm::Value* value, llvm::Type* targetType);
    llvm::Value* convertArray(llvm::Value* array, llvm::Type* targetType);

    // Prevent copying
    TypeHelper(const TypeHelper&) = delete;
    TypeHelper& operator=(const TypeHelper&) = delete;
};

#endif // TYPE_HELPER_H