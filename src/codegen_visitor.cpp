#include "codegen_visitor.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Verifier.h>

void CodegenVisitor::reportError(const std::string& message, const Location& loc) {
    std::string context;
    if (currentFunction) {
        context = " in function '" + currentFunction->getName().str() + "'";
    }
    
    errorHandler.error(
        ErrorLevel::CODEGEN,
        loc.line,
        loc.column,
        message + context
    );
    
    // Print current compilation state for debugging
    if (module) {
        std::string state;
        llvm::raw_string_ostream stateStream(state);
        module->print(stateStream, nullptr);
        errorHandler.error(
            ErrorLevel::CODEGEN,
            loc.line,
            loc.column,
            "Current module state:\n" + state
        );
    }
}


CodegenVisitor::CodegenVisitor(llvm::LLVMContext& ctx)
    : context(ctx),
      builder(std::make_unique<llvm::IRBuilder<>>(ctx)),
      currentFunction(nullptr),
      lastValue(nullptr) {}

      
std::unique_ptr<llvm::Module> CodegenVisitor::generateModule(Program* program, const std::string& moduleName) {
    if (!program) {
        errorHandler.error(ErrorLevel::CODEGEN, 0, 0, "Null program passed to code generator");
        return nullptr;
    }

    try {
        module = std::make_unique<llvm::Module>(moduleName, context);
        if (!module) {
            errorHandler.error(ErrorLevel::CODEGEN, 0, 0, "Failed to create LLVM module");
            return nullptr;
        }

        // Declare runtime functions first
        declareRuntimeFunctions();

        // Generate code for the program
        program->accept(this);
    
        // Print module state before execution
        std::cout << "Debug: Module IR before execution:\n";
        module->print(llvm::outs(), nullptr);
        // Verify the module
        std::string error;
        llvm::raw_string_ostream errorStream(error);
        if (llvm::verifyModule(*module, &errorStream)) {
            errorHandler.error(
                ErrorLevel::CODEGEN,
                0, 0,
                "Module verification failed: " + error
            );
            return nullptr;
        }

        return std::move(module);
    } catch (const std::exception& e) {
        errorHandler.error(
            ErrorLevel::CODEGEN,
            0, 0,
            std::string("Internal error during code generation: ") + e.what()
        );
        return nullptr;
    }
}


llvm::Type* CodegenVisitor::getLLVMType(const Type& type) {
    if (type.name == "int") {
        return llvm::Type::getInt32Ty(context);
    } else if (type.name == "float") {
        return llvm::Type::getDoubleTy(context);
    } else if (type.name == "bool") {
        return llvm::Type::getInt1Ty(context);
    } else if (type.name == "str") {
        // Strings are represented as i8* (char*)
        return llvm::Type::getInt8PtrTy(context);
    } else if (type.isArray) {
        llvm::Type* elemType = getLLVMType(Type(type.name));
        if (!elemType) return nullptr;
        return llvm::PointerType::get(elemType, 0);
    }

    reportError("Unknown type: " + type.name, Location());
    return nullptr;
}

llvm::Value* CodegenVisitor::generateAlloca(llvm::Function* function, const std::string& name, llvm::Type* type) {
    // Create IRBuilder for the entry block
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(), 
                                function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}


llvm::Value* CodegenVisitor::convertValue(llvm::Value* value, llvm::Type* targetType) {
    if (!value || !targetType) return nullptr;
    
    if (value->getType() == targetType) return value;
    
    // Integer to floating point conversion
    if (value->getType()->isIntegerTy() && targetType->isDoubleTy()) {
        return builder->CreateSIToFP(value, targetType, "conv");
    }
    
    // Floating point to integer conversion
    if (value->getType()->isDoubleTy() && targetType->isIntegerTy()) {
        return builder->CreateFPToSI(value, targetType, "conv");
    }
    
    return nullptr;
}

void CodegenVisitor::visit(Program* node) {
    if (!node) {
        reportError("Null program node", Location());
        return;
    }

    std::cout << "Debug: Starting function declarations\n";

    // First pass: declare all functions
    for (const auto& func : node->functions) {
        if (!func) continue;
        
        std::cout << "Debug: Processing function: " << func->name.value << "\n";
        
        auto symbol = symbolTable.resolveFunction(func->name.value);
        if (!symbol) {
            reportError("Function symbol not found: " + func->name.value, func->loc);
            continue;
        }

        // Create function type
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : func->parameters) {
            if (auto paramType = getLLVMType(param.type)) {
                paramTypes.push_back(paramType);
            } else {
                reportError("Invalid parameter type in function: " + func->name.value, func->loc);
                return;
            }
        }

        llvm::Type* returnType = getLLVMType(func->returnType);
        if (!returnType) {
            reportError("Invalid return type for function: " + func->name.value, func->loc);
            return;
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
        llvm::Function* function = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, func->name.value, module.get()
        );

         std::cout << "Debug: Created LLVM function: " << function->getName().str() << "\n";
        // Store LLVM function in symbol table
        symbol->llvmFunction = function;
    }


    // Second pass: generate function bodies
    std::cout << "Debug: Generating function bodies\n";
    for (const auto& func : node->functions) {
        if (!func) continue;
        std::cout << "Debug: Generating body for: " << func->name.value << "\n";
        func->accept(this);
    }
}
void CodegenVisitor::visit(FunctionDecl* node) {
    // Check if the function already exists in the module
    llvm::Function* function = module->getFunction(node->name.value);
    if (!function) {
        // Create function type
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : node->parameters) {
            llvm::Type* paramType = getLLVMType(param.type);
            if (!paramType) {
                reportError("Invalid parameter type for " + param.name.value,
            Location(param.name.line, param.name.column));
                return;
            }
            paramTypes.push_back(paramType);
        }

        llvm::Type* returnType = getLLVMType(node->returnType);
        if (!returnType) {
            reportError("Invalid return type for function " + node->name.value, node->loc);
            return;
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
        function = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, node->name.value, module.get()
        );
    }

    // Save the current function and create entry block
    currentFunction = function;
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", function);
    builder->SetInsertPoint(entry);

    // Enter function scope and map parameters
    symbolTable.enterScope();
    auto argIt = function->arg_begin();
    for (const auto& param : node->parameters) {
        llvm::Type* paramType = getLLVMType(param.type);
        if (!paramType) {
            reportError("Invalid parameter type", node->loc);
            return;
        }

        // Declare parameter in symbol table
        if (!symbolTable.declare(param.name.value, param.type)) {
            reportError("Parameter redeclaration: " + param.name.value, node->loc);
            return;
        }

        // Allocate storage and store the argument
        llvm::Value* alloca = generateAlloca(function, param.name.value, paramType);
        builder->CreateStore(&*argIt, alloca);

        // Update symbol table with LLVM allocation
        auto symbol = symbolTable.resolve(param.name.value);
        if (!symbol) {
            reportError("Failed to resolve parameter: " + param.name.value,
            Location(param.name.line, param.name.column));
            return;
        }
        symbol->llvmValue = alloca;
        symbol->isAlloca = true;

        argIt++;
    }

    // Generate the function body
    node->body->accept(this);

    // Ensure the function has a return terminator
    if (!builder->GetInsertBlock()->getTerminator()) {
        llvm::Type* returnType = currentFunction->getReturnType();
        if (returnType->isVoidTy()) {
            builder->CreateRetVoid();
        } else {
            builder->CreateRet(llvm::Constant::getNullValue(returnType));
        }
    }

    // Exit function scope and clear the current function
    symbolTable.exitScope();
    currentFunction = nullptr;
}



void CodegenVisitor::visit(NumberExpr* node) {
    if (node->isFloat) {
        lastValue = llvm::ConstantFP::get(context, llvm::APFloat(std::stod(node->token.value)));
    } else {
        lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, std::stoi(node->token.value), true));
    }
}


void CodegenVisitor::visit(StringExpr* node) {
    // Check if we've already created this string constant
    auto it = stringConstants.find(node->token.value);
    if (it != stringConstants.end()) {
        lastValue = it->second;
        return;
    }

    // Create a new global string constant with proper null termination
    llvm::Constant* strConstant = builder->CreateGlobalStringPtr(
        node->token.value,
        "str",
        0  // No constant alignment
    );
    
    stringConstants[node->token.value] = strConstant;
    lastValue = strConstant;
}

void CodegenVisitor::visit(BoolExpr* node) {
    lastValue = llvm::ConstantInt::get(context, llvm::APInt(1, node->value ? 1 : 0));
}

void CodegenVisitor::visit(VariableExpr* node) {
    Symbol* symbol = symbolTable.resolve(node->name.value);
    if (!symbol || !symbol->llvmValue) {
        reportError("Undefined variable: " + node->name.value, node->loc);
        lastValue = nullptr;
        return;
    }

    if (symbol->isAlloca) {
        lastValue = builder->CreateLoad(getLLVMType(symbol->type), symbol->llvmValue);
    } else {
        lastValue = symbol->llvmValue;
    }
}

void CodegenVisitor::visit(BinaryExpr* node) {
    node->left->accept(this);
    llvm::Value* left = lastValue;

    node->right->accept(this);
    llvm::Value* right = lastValue;

    if (!left || !right) {
        lastValue = nullptr;
        return;
    }

    bool isFloat = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();
    if (isFloat) {
        left = convertValue(left, llvm::Type::getDoubleTy(context));
        right = convertValue(right, llvm::Type::getDoubleTy(context));
    }

    switch (node->op.type) {
        case PLUS:
            lastValue = isFloat ? 
                builder->CreateFAdd(left, right, "addtmp") :
                builder->CreateAdd(left, right, "addtmp");
            break;

        case MINUS:
            lastValue = isFloat ?
                builder->CreateFSub(left, right, "subtmp") :
                builder->CreateSub(left, right, "subtmp");
            break;

        case STAR:
            lastValue = isFloat ?
                builder->CreateFMul(left, right, "multmp") :
                builder->CreateMul(left, right, "multmp");
            break;

        case SLASH:
            lastValue = isFloat ?
                builder->CreateFDiv(left, right, "divtmp") :
                builder->CreateSDiv(left, right, "divtmp");
            break;

        case EQUALS_EQUALS:
            lastValue = isFloat ?
                builder->CreateFCmpOEQ(left, right, "eqtmp") :
                builder->CreateICmpEQ(left, right, "eqtmp");
            break;

        case NOT_EQUALS:
            lastValue = isFloat ?
                builder->CreateFCmpONE(left, right, "netmp") :
                builder->CreateICmpNE(left, right, "netmp");
            break;

        case GREATER_EQUAL:  // Handle `>=`
            lastValue = isFloat
                ? builder->CreateFCmpOGE(left, right, "cmpgetmp")
                : builder->CreateICmpSGE(left, right, "cmpgetmp");
            break;

        case LESS_EQUAL:  // Handle `<=`
            lastValue = isFloat
                ? builder->CreateFCmpOLE(left, right, "cmpletmp")
                : builder->CreateICmpSLE(left, right, "cmpletmp");
            break;

        case LESS:
            lastValue = isFloat ?
                builder->CreateFCmpOLT(left, right, "lttmp") :
                builder->CreateICmpSLT(left, right, "lttmp");
            break;

        case GREATER:
            lastValue = isFloat ?
                builder->CreateFCmpOGT(left, right, "gttmp") :
                builder->CreateICmpSGT(left, right, "gttmp");
            break;

        case AND:
            lastValue = builder->CreateAnd(left, right, "andtmp");
            break;

        case OR:
            lastValue = builder->CreateOr(left, right, "ortmp");
            break;

        default:
            reportError("Unknown binary operator", node->loc);
            lastValue = nullptr;
    }
}

void CodegenVisitor::visit(UnaryExpr* node) {
    node->expr->accept(this);
    llvm::Value* operand = lastValue;
    
    if (!operand) {
        lastValue = nullptr;
        return;
    }

    switch (node->op.type) {
        case MINUS:
            if (operand->getType()->isDoubleTy()) {
                lastValue = builder->CreateFNeg(operand, "negtmp");
            } else {
                lastValue = builder->CreateNeg(operand, "negtmp");
            }
            break;

        case NOT:
            lastValue = builder->CreateNot(operand, "nottmp");
            break;

        default:
            reportError("Unknown unary operator", node->loc);
            lastValue = nullptr;
    }
}

void CodegenVisitor::visit(ArrayAccessExpr* node) {
    node->array->accept(this);
    llvm::Value* array = lastValue;
    
    node->index->accept(this);
    llvm::Value* index = lastValue;
    
    if (!array || !index) {
        reportError("Invalid array access", node->loc);
        lastValue = nullptr;
        return;
    }
    
    // Create the GEP instruction for array access
    std::vector<llvm::Value*> indices = {index};
    lastValue = builder->CreateGEP(array->getType()->getPointerElementType(), array, indices, "arrayaccess");
    
    // Load the value unless this is the target of an assignment
    lastValue = builder->CreateLoad(lastValue->getType()->getPointerElementType(), lastValue, "arrayvalue");
}

void CodegenVisitor::declareRuntimeFunctions() {
    // Declare printf
    llvm::FunctionType* printfTy = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context),
        {llvm::Type::getInt8PtrTy(context)},
        true  // varargs
    );
    module->getOrInsertFunction("printf", printfTy);

    // Declare stdin
    module->getOrInsertGlobal("stdin", 
        llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0));

    // Declare fgets
    llvm::FunctionType* fgetsTy = llvm::FunctionType::get(
        llvm::Type::getInt8PtrTy(context),
        {
            llvm::Type::getInt8PtrTy(context),  // buffer
            llvm::Type::getInt32Ty(context),    // size
            llvm::Type::getInt8PtrTy(context)   // stream
        },
        false
    );
    module->getOrInsertFunction("fgets", fgetsTy);

    // Declare strlen
    llvm::FunctionType* strlenTy = llvm::FunctionType::get(
        llvm::Type::getInt64Ty(context),
        {llvm::Type::getInt8PtrTy(context)},
        false
    );
    module->getOrInsertFunction("strlen", strlenTy);

    // Memory management functions
    llvm::FunctionType* mallocTy = llvm::FunctionType::get(
        llvm::Type::getInt8PtrTy(context),
        {llvm::Type::getInt64Ty(context)},
        false
    );
    module->getOrInsertFunction("malloc", mallocTy);

    llvm::FunctionType* freeTy = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context),
        {llvm::Type::getInt8PtrTy(context)},
        false
    );
    module->getOrInsertFunction("free", freeTy);

    llvm::FunctionType* reallocTy = llvm::FunctionType::get(
        llvm::Type::getInt8PtrTy(context),
        {
            llvm::Type::getInt8PtrTy(context),
            llvm::Type::getInt64Ty(context)
        },
        false
    );
    module->getOrInsertFunction("realloc", reallocTy);
}

void CodegenVisitor::visit(CallExpr* node) {
    if (!node) {
        reportError("Null call expression", Location());
        return;
    }

    // Handle built-in functions
    if (node->name.value == "print") {
        generatePrintCall(node);
        return;
    } else if (node->name.value == "input") {
        generateInputCall(node);
        return;
    } else if (node->name.value == "malloc") {
        generateMallocCall(node);
        return;
    } else if (node->name.value == "free") {
        generateFreeCall(node);
        return;
    } else if (node->name.value == "realloc") {
        generateReallocCall(node);
        return;
    } else if (node->name.value == "sizeof") {
        generateSizeofCall(node);
        return;
    }

    // Handle regular function calls
    auto* funcSymbol = symbolTable.resolveFunction(node->name.value);
    if (!funcSymbol || !funcSymbol->llvmFunction) {
        reportError("Undefined function: " + node->name.value, node->loc);
        return;
    }

    // Evaluate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : node->arguments) {
        if (!arg) continue;
        arg->accept(this);
        if (!lastValue) {
            reportError("Invalid function argument", arg->loc);
            return;
        }
        args.push_back(lastValue);
    }

    // Create the call instruction
    lastValue = builder->CreateCall(funcSymbol->llvmFunction, args);
}

void CodegenVisitor::generatePrintCall(CallExpr* node) {
    if (node->arguments.empty()) {
        reportError("print() requires an argument", node->loc);
        lastValue = nullptr;
        return;
    }

    llvm::Function* printfFunc = module->getFunction("printf");
    if (!printfFunc) {
        reportError("printf function not found", node->loc);
        return;
    }

    node->arguments[0]->accept(this);
    llvm::Value* arg = lastValue;
    if (!arg) return;

    std::vector<llvm::Value*> printfArgs;
    llvm::Value* formatStr = nullptr;

    if (arg->getType()->isIntegerTy(32)) {
        formatStr = builder->CreateGlobalStringPtr("%d\n");
    } else if (arg->getType()->isDoubleTy()) {
        formatStr = builder->CreateGlobalStringPtr("%f\n");
    } else if (arg->getType()->isPointerTy() && 
               arg->getType()->getPointerElementType()->isIntegerTy(8)) {
        // String type (i8*)
        formatStr = builder->CreateGlobalStringPtr("%s\n");
    } else if (arg->getType()->isIntegerTy(1)) {  // boolean
        // For booleans, we'll use select to choose between "true\n" and "false\n"
        llvm::Value* trueStr = builder->CreateGlobalStringPtr("true\n");
        llvm::Value* falseStr = builder->CreateGlobalStringPtr("false\n");
        formatStr = builder->CreateSelect(arg, trueStr, falseStr);
        
        // For boolean, we directly use the string without format
        printfArgs.push_back(formatStr);
        lastValue = builder->CreateCall(printfFunc, printfArgs);
        return;
    } else {
        reportError("Unsupported type for print()", node->loc);
        lastValue = nullptr;
        return;
    }

    printfArgs.push_back(formatStr);
    printfArgs.push_back(arg);
    lastValue = builder->CreateCall(printfFunc, printfArgs);
}


// In codegen_visitor.cpp

void CodegenVisitor::generateInputCall(CallExpr* node) {
    // Get required functions
    llvm::Function* fgetsFunc = module->getFunction("fgets");
    llvm::Function* strlenFunc = module->getFunction("strlen");
    llvm::Value* stdinValue = module->getNamedGlobal("stdin");

    // Create buffer type and allocation
    llvm::ArrayType* bufferType = llvm::ArrayType::get(llvm::Type::getInt8Ty(context), 1024);
    llvm::Value* buffer = builder->CreateAlloca(bufferType, nullptr, "input_buffer");

    // Handle prompt if provided
    if (!node->arguments.empty()) {
        node->arguments[0]->accept(this);
        if (lastValue) {
            llvm::Function* printfFunc = module->getFunction("printf");
            llvm::Value* formatStr = builder->CreateGlobalStringPtr("%s");
            builder->CreateCall(printfFunc, {formatStr, lastValue});
        }
    }

    // Load stdin
    llvm::Value* stdin = builder->CreateLoad(
        llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
        stdinValue
    );

    // Convert buffer to i8* for fgets
    llvm::Value* bufferPtr = builder->CreateBitCast(
        buffer, 
        llvm::Type::getInt8PtrTy(context)
    );

    // Call fgets(buffer, 1024, stdin)
    builder->CreateCall(fgetsFunc, {
        bufferPtr,
        llvm::ConstantInt::get(context, llvm::APInt(32, 1024)),
        stdin
    });

    // Get string length
    llvm::Value* len = builder->CreateCall(strlenFunc, {bufferPtr});

    // Compute pointer to last character
    std::vector<llvm::Value*> indices;
    indices.push_back(llvm::ConstantInt::get(context, llvm::APInt(32, 0))); // Base array index
    indices.push_back(builder->CreateSub(
        builder->CreateTrunc(len, llvm::Type::getInt32Ty(context)),
        llvm::ConstantInt::get(context, llvm::APInt(32, 1))
    ));

    // Get pointer to last character using GEP
    llvm::Value* lastCharPtr = builder->CreateInBoundsGEP(
        bufferType,
        buffer,
        indices,
        "last_char_ptr"
    );

    // Load last character
    llvm::Value* lastChar = builder->CreateLoad(
        llvm::Type::getInt8Ty(context),
        lastCharPtr,
        "last_char"
    );

    // Check if it's a newline
    llvm::Value* isNewline = builder->CreateICmpEQ(
        lastChar,
        llvm::ConstantInt::get(context, llvm::APInt(8, '\n'))
    );

    // Create blocks for newline removal
    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, "remove_newline", currentFunction);
    llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(context, "continue", currentFunction);

    // Branch based on newline check
    builder->CreateCondBr(isNewline, thenBlock, continueBlock);

    // Remove newline if present
    builder->SetInsertPoint(thenBlock);
    builder->CreateStore(
        llvm::ConstantInt::get(context, llvm::APInt(8, 0)),
        lastCharPtr
    );
    builder->CreateBr(continueBlock);

    // Continue point
    builder->SetInsertPoint(continueBlock);

    // Return the buffer pointer
    lastValue = bufferPtr;
}

llvm::Value* CodegenVisitor::generateSizeofCall(CallExpr* node) {
    if (node->arguments.size() != 1) {
        reportError("sizeof() requires exactly one argument", node->loc);
        return nullptr;
    }

    Expr* arg = node->arguments[0].get();
    if (auto* typeExpr = dynamic_cast<TypeExpr*>(arg)) {
        typeExpr->accept(this);
        return lastValue;  // `lastValue` now contains the size
    }

    reportError("sizeof() argument must be a type expression", node->loc);
    return nullptr;
}



void CodegenVisitor::generateMallocCall(CallExpr* node) {
    if (node->arguments.size() != 1) {
        reportError("malloc() requires exactly one size argument", node->loc);
        lastValue = nullptr;
        return;
    }

    node->arguments[0]->accept(this);
    if (!lastValue) return;

    llvm::Function* mallocFunc = module->getFunction("malloc");
    lastValue = builder->CreateCall(mallocFunc, {lastValue});
}

void CodegenVisitor::generateFreeCall(CallExpr* node) {
    if (node->arguments.size() != 1) {
        reportError("free() requires exactly one pointer argument", node->loc);
        lastValue = nullptr;
        return;
    }

    node->arguments[0]->accept(this);
    if (!lastValue) return;

    llvm::Function* freeFunc = module->getFunction("free");
    lastValue = builder->CreateCall(freeFunc, {lastValue});
}

void CodegenVisitor::generateReallocCall(CallExpr* node) {
    if (node->arguments.size() != 2) {
        reportError("realloc() requires pointer and size arguments", node->loc);
        lastValue = nullptr;
        return;
    }

    node->arguments[0]->accept(this);
    llvm::Value* ptr = lastValue;
    if (!ptr) return;

    node->arguments[1]->accept(this);
    llvm::Value* size = lastValue;
    if (!size) return;

    llvm::Function* reallocFunc = module->getFunction("realloc");
    lastValue = builder->CreateCall(reallocFunc, {ptr, size});
}
void CodegenVisitor::visit(AssignExpr* node) {
    node->value->accept(this);
    llvm::Value* value = lastValue;
    
    // Handle the target
    if (auto* var = dynamic_cast<VariableExpr*>(node->target.get())) {
        Symbol* symbol = symbolTable.resolve(var->name.value);
        if (!symbol || !symbol->llvmValue) {
            reportError("Undefined variable: " + var->name.value, node->loc);
            lastValue = nullptr;
            return;
        }
        value = convertValue(value, getLLVMType(symbol->type));
        builder->CreateStore(value, symbol->llvmValue);
        lastValue = value;
    }
    else if (auto* arrayAccess = dynamic_cast<ArrayAccessExpr*>(node->target.get())) {
        arrayAccess->array->accept(this);
        llvm::Value* array = lastValue;
        arrayAccess->index->accept(this);
        llvm::Value* index = lastValue;
        
        if (!array || !index) {
            reportError("Invalid array access in assignment", node->loc);
            lastValue = nullptr;
            return;
        }
        
        std::vector<llvm::Value*> indices = {index};
        llvm::Value* elementPtr = builder->CreateGEP(array->getType()->getPointerElementType(), 
                                                   array, indices, "arrayelem");
        value = convertValue(value, elementPtr->getType()->getPointerElementType());
        builder->CreateStore(value, elementPtr);
        lastValue = value;
    }
    else {
        reportError("Invalid assignment target", node->loc);
        lastValue = nullptr;
    }
}

void CodegenVisitor::visit(ArrayInitExpr* node) {
    // Get size of the array
    size_t size = node->elements.size();
    
    // Create array type based on first element if available
    llvm::Type* elementType = nullptr;
    if (!node->elements.empty()) {
        node->elements[0]->accept(this);
        elementType = lastValue->getType();
    } else {
        elementType = llvm::Type::getInt32Ty(context); // Default to int
    }
    
    // Allocate array
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, size);
    llvm::Value* array = builder->CreateAlloca(arrayType, nullptr, "array");
    
    // Initialize elements
    for (size_t i = 0; i < node->elements.size(); i++) {
        node->elements[i]->accept(this);
        if (!lastValue) continue;
        
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, i))
        };
        llvm::Value* elementPtr = builder->CreateGEP(arrayType, array, indices);
        builder->CreateStore(lastValue, elementPtr);
    }
    
    lastValue = array;
}

void CodegenVisitor::visit(ArrayAllocExpr* node) {
    node->size->accept(this);
    llvm::Value* size = lastValue;
    
    if (!size) {
        reportError("Invalid array size", node->loc);
        lastValue = nullptr;
        return;
    }
    
    llvm::Type* elementType = getLLVMType(node->elementType);
    if (!elementType) {
        reportError("Invalid array element type", node->loc);
        lastValue = nullptr;
        return;
    }
    
    // Get DataLayout from the module and calculate element size
    const llvm::DataLayout& dataLayout = module->getDataLayout();
    llvm::Value* elementSize = llvm::ConstantInt::get(context, 
        llvm::APInt(64, dataLayout.getTypeAllocSize(elementType)));
    
    // Calculate total size in bytes
    llvm::Value* totalSize = builder->CreateMul(
        builder->CreateZExt(size, llvm::Type::getInt64Ty(context)),
        elementSize
    );
    
    // Call malloc
    llvm::Function* mallocFunc = module->getFunction("malloc");
    llvm::Value* memory = builder->CreateCall(mallocFunc, {totalSize});
    
    // Bitcast to correct pointer type
    lastValue = builder->CreateBitCast(memory, elementType->getPointerTo());
}

void CodegenVisitor::visit(TypeExpr* node) {
    // Type expressions don't generate code directly
    lastValue = nullptr;
}

void CodegenVisitor::visit(ExprStmt* node) {
    node->expr->accept(this);
    // Expression statements don't need to retain their value
    lastValue = nullptr;
}


void CodegenVisitor::visit(VarDeclStmt* node) {
    llvm::Type* type = getLLVMType(node->type);
    if (!type) {
        reportError("Invalid variable type", node->loc);
        return;
    }

    if (!currentFunction) {
        reportError("Variable declaration outside function", node->loc);
        return;
    }

    // Create alloca instruction at the beginning of the function
    llvm::Value* alloca = generateAlloca(currentFunction, node->name.value, type);
    if (!alloca) {
        reportError("Failed to allocate storage for variable: " + node->name.value, node->loc);
        return;
    }

    // Store the alloca in the symbol table
    auto symbol = symbolTable.resolve(node->name.value);
    if (!symbol) {
        if (!symbolTable.declare(node->name.value, node->type)) {
            reportError("Failed to declare symbol: " + node->name.value, node->loc);
            return;
        }
        symbol = symbolTable.resolve(node->name.value);
    }

    symbol->llvmValue = alloca;
    symbol->isAlloca = true;

    // Handle initializer if present
    if (node->initializer) {
        node->initializer->accept(this);
        if (!lastValue) {
            reportError("Invalid initializer for variable: " + node->name.value, node->loc);
            return;
        }

        // Special handling for string initialization
        if (node->type.name == "str") {
            if (lastValue->getType() != type) {
                lastValue = builder->CreateBitCast(lastValue, type);
            }
        } else {
            lastValue = convertValue(lastValue, type);
        }

        builder->CreateStore(lastValue, alloca);
    } else {
        // Initialize with default value
        llvm::Value* defaultValue;
        if (node->type.name == "str") {
            // Initialize string to empty string
            defaultValue = builder->CreateGlobalStringPtr("");
        } else if (type->isIntegerTy()) {
            defaultValue = llvm::ConstantInt::get(type, 0);
        } else if (type->isDoubleTy()) {
            defaultValue = llvm::ConstantFP::get(type, 0.0);
        } else if (type->isPointerTy()) {
            defaultValue = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
        } else {
            defaultValue = llvm::Constant::getNullValue(type);
        }
        builder->CreateStore(defaultValue, alloca);
    }
}


void CodegenVisitor::visit(BlockStmt* node) {
    for (const auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void CodegenVisitor::visit(IfStmt* node) {
    // Generate condition
    node->condition->accept(this);
    llvm::Value* condition = lastValue;
    
    if (!condition) return;
    
    // Create basic blocks
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context, "then", func);
    llvm::BasicBlock* elseBB = node->elseBranch ? llvm::BasicBlock::Create(context, "else") : nullptr;
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context, "ifcont");

    // Branch to the appropriate block based on condition
    builder->CreateCondBr(condition, thenBB, elseBB ? elseBB : mergeBB);

    // Generate 'then' block
    builder->SetInsertPoint(thenBB);
    node->thenBranch->accept(this);
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);  // Add branch only if the block is not already terminated
    }

    // Generate 'else' block if it exists
    if (elseBB) {
        func->getBasicBlockList().push_back(elseBB);
        builder->SetInsertPoint(elseBB);
        node->elseBranch->accept(this);
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(mergeBB);
        }
    }

    // Merge block
    func->getBasicBlockList().push_back(mergeBB);
    builder->SetInsertPoint(mergeBB);
}


void CodegenVisitor::visit(WhileStmt* node) {
    llvm::Function* func = builder->GetInsertBlock()->getParent();
    
    // Create basic blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(context, "whilecond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context, "whilebody", func);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(context, "whileend", func);
    
    // Jump to condition block
    builder->CreateBr(condBB);
    
    // Generate condition code
    builder->SetInsertPoint(condBB);
    node->condition->accept(this);
    if (!lastValue) return;
    
    llvm::Value* condition = builder->CreateICmpNE(
        lastValue,
        llvm::ConstantInt::get(lastValue->getType(), 0),
        "whilecond"
    );
    builder->CreateCondBr(condition, bodyBB, endBB);
    
    // Generate body code
    builder->SetInsertPoint(bodyBB);
    node->body->accept(this);
    builder->CreateBr(condBB);
    
    // Continue with end block
    builder->SetInsertPoint(endBB);
}

void CodegenVisitor::visit(ReturnStmt* node) {
    if (!node->value) {
        builder->CreateRetVoid();
        lastValue = nullptr;
        return;
    }
    
    node->value->accept(this);
    if (!lastValue) return;
    
    // Convert return value to function's return type if needed
    llvm::Type* returnType = currentFunction->getReturnType();
    lastValue = convertValue(lastValue, returnType);
    
    builder->CreateRet(lastValue);
}