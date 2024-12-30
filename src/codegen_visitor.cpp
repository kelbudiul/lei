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
    llvm::Type* baseType = nullptr;
    
    // Get the base type first
    if (type.name == "int") {
        baseType = llvm::Type::getInt32Ty(context);
    } else if (type.name == "void") {
     baseType = llvm::Type::getVoidTy(context);
    }
    else if (type.name == "float") {
        baseType = llvm::Type::getDoubleTy(context);
    } else if (type.name == "bool") {
        baseType = llvm::Type::getInt1Ty(context);
    } else if (type.name == "str") {
        baseType = llvm::Type::getInt8PtrTy(context);
    } else {
        reportError("Unknown type: " + type.name, Location());
        return nullptr;
    }
    
    // Now handle array types
    if (type.isArray) {
        if (type.arraySize >= 0) {
            // Fixed size array
            return llvm::ArrayType::get(baseType, type.arraySize);
        } else {
            // Dynamic array - pointer to base type
            return llvm::PointerType::get(baseType, 0);
        }
    }
    
    return baseType;
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
    // Generate code for array base
    node->array->accept(this);
    llvm::Value* arrayPtr = lastValue;

    // Generate code for index
    node->index->accept(this);
    llvm::Value* index = lastValue;

    if (!arrayPtr || !index) {
        reportError("Invalid array access", node->loc);
        return;
    }

    // If we got a load instruction, get the pointer operand
    if (auto* loadInst = llvm::dyn_cast<llvm::LoadInst>(arrayPtr)) {
        arrayPtr = loadInst->getPointerOperand();
    }

    llvm::Type* ptrType = arrayPtr->getType();
    llvm::Type* elementType = nullptr;

    // Handle different array types
    if (ptrType->isPointerTy()) {
        if (ptrType->getPointerElementType()->isArrayTy()) {
            // Fixed-size array
            elementType = ptrType->getPointerElementType()->getArrayElementType();
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
                index
            };
            
            lastValue = builder->CreateInBoundsGEP(
                ptrType->getPointerElementType(),
                arrayPtr,
                indices,
                "array.element"
            );
        }
        else if (ptrType->getPointerElementType()->isPointerTy()) {
            // Dynamic array (pointer to pointer)
            elementType = ptrType->getPointerElementType()->getPointerElementType();
            llvm::Value* ptr = builder->CreateLoad(ptrType->getPointerElementType(), arrayPtr);
            lastValue = builder->CreateInBoundsGEP(elementType, ptr, index, "array.element");
        }
        else {
            // Dynamic array (direct pointer)
            elementType = ptrType->getPointerElementType();
            lastValue = builder->CreateInBoundsGEP(elementType, arrayPtr, index, "array.element");
        }
    }
    else {
        reportError("Invalid array type", node->loc);
        return;
    }

    // If this is not an assignment target, load the value
    if (!isAssignmentTarget && lastValue) {
        lastValue = builder->CreateLoad(lastValue->getType()->getPointerElementType(), 
                                     lastValue, 
                                     "array.value");
    }
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


    llvm::FunctionType* strchrTy = llvm::FunctionType::get(
        llvm::Type::getInt8PtrTy(context),
        {llvm::Type::getInt8PtrTy(context), llvm::Type::getInt8Ty(context)},
        false
    );
    module->getOrInsertFunction("strchr", strchrTy);

    // Declare atoi
    llvm::FunctionType* atoiType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(context), {llvm::Type::getInt8PtrTy(context)}, false);
    module->getOrInsertFunction("atoi", atoiType);

    // Declare atof
    llvm::FunctionType* atofType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(context), {llvm::Type::getInt8PtrTy(context)}, false);
    module->getOrInsertFunction("atof", atofType);

    // Declare itoa (requires a buffer for simplicity)
    llvm::FunctionType* itoaType = llvm::FunctionType::get(
        llvm::Type::getInt8PtrTy(context),
        {llvm::Type::getInt32Ty(context), llvm::Type::getInt8PtrTy(context), llvm::Type::getInt32Ty(context)},
        false);
    module->getOrInsertFunction("itoa", itoaType);

    // Declare ftoa (requires a buffer for simplicity)
    llvm::FunctionType* ftoaType = llvm::FunctionType::get(
        llvm::Type::getInt8PtrTy(context),
        {llvm::Type::getDoubleTy(context), llvm::Type::getInt8PtrTy(context), llvm::Type::getInt32Ty(context)},
        false);
    module->getOrInsertFunction("ftoa", ftoaType);
}

llvm::Value* CodegenVisitor::handleArrayArgument(llvm::Value* arrayValue) {
    llvm::Type* arrayType = arrayValue->getType();
    if (arrayType->isPointerTy() && arrayType->getPointerElementType()->isArrayTy()) {
        // Extract the pointer to the first element
        llvm::Value* firstElementPtr = builder->CreateInBoundsGEP(
            arrayType->getPointerElementType(), // Array type
            arrayValue,                        // Base pointer
            {llvm::ConstantInt::get(context, llvm::APInt(32, 0)), // Array index 0
             llvm::ConstantInt::get(context, llvm::APInt(32, 0))}, // Element index 0
            "arrayptr"
        );
        return firstElementPtr;
    }
    return arrayValue; // Not an array, return as-is
}


void CodegenVisitor::visit(CallExpr* node) {
    if (!node) {
        reportError("Null call expression", Location());
        return;
    }

    llvm::Function* func = nullptr;

    // Handle built-in functions
    if (node->name.value == "atoi" || node->name.value == "atof" ||
        node->name.value == "itoa" || node->name.value == "ftoa") {
        func = module->getFunction(node->name.value);
    } else if (node->name.value == "print") {
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
    } else if (node->name.value == "strlen") {
    generateStrlenCall(node);
    return;
    }

    // Handle built-in conversion functions (atoi, atof, etc.)
    if (func) {
        if (node->arguments.size() != 1) {
            reportError("Function " + node->name.value + " expects one argument", node->loc);
            return;
        }

        // Process single argument
        node->arguments[0]->accept(this);
        if (!lastValue) {
            reportError("Invalid argument for function " + node->name.value, node->loc);
            return;
        }

        // Generate call with single argument
        lastValue = builder->CreateCall(func, {lastValue}, "calltmp");
        return;
    }

    // Handle regular function calls
    auto* funcSymbol = symbolTable.resolveFunction(node->name.value);
    if (!funcSymbol || !funcSymbol->llvmFunction) {
        reportError("Undefined function: " + node->name.value, node->loc);
        return;
    }

    // Process arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : node->arguments) {
        arg->accept(this);  // Generate code for the argument
        llvm::Value* value = lastValue;

        if (!value) {
            reportError("Invalid function argument", arg->loc);
            return;
        }

        // Check and handle array argument
        value = handleArrayArgument(value);
        args.push_back(value);
        
    }

    // Generate the function call
    lastValue = builder->CreateCall(funcSymbol->llvmFunction, args);
}

void CodegenVisitor::generateStrlenCall(CallExpr* node) {
    if (node->arguments.size() != 1) {
        reportError("strlen() requires exactly one string argument", node->loc);
        lastValue = nullptr;
        return;
    }

    // Generate code for the argument
    node->arguments[0]->accept(this);
    llvm::Value* str = lastValue;
    if (!str) return;

    // Call strlen
    llvm::Function* strlenFunc = module->getFunction("strlen");
    if (!strlenFunc) {
        reportError("strlen function not found", node->loc);
        lastValue = nullptr;
        return;
    }

    // Create the call and truncate the result to 32 bits (since we use int32 for our int type)
    llvm::Value* result = builder->CreateCall(strlenFunc, {str});
    lastValue = builder->CreateTrunc(result, llvm::Type::getInt32Ty(context), "strlen.result");
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

    // Generate code for the argument
    node->arguments[0]->accept(this);
    llvm::Value* arg = lastValue;
    if (!arg) return;

    // Handle loaded values
    if (auto* loadInst = llvm::dyn_cast<llvm::LoadInst>(arg)) {
        arg = loadInst;
    }

    std::vector<llvm::Value*> printfArgs;
    llvm::Value* formatStr = nullptr;

    // Handle different types
    if (arg->getType()->isIntegerTy(32)) {
        formatStr = builder->CreateGlobalStringPtr("%d\n");
        printfArgs = {formatStr, arg};
    } 
    else if (arg->getType()->isDoubleTy()) {
        formatStr = builder->CreateGlobalStringPtr("%f\n");
        printfArgs = {formatStr, arg};
    }
    else if (arg->getType()->isPointerTy() && 
             arg->getType()->getPointerElementType()->isIntegerTy(8)) {
        formatStr = builder->CreateGlobalStringPtr("%s\n");
        printfArgs = {formatStr, arg};
    }
    else if (arg->getType()->isIntegerTy(1)) {
        formatStr = builder->CreateGlobalStringPtr("%s\n");
        llvm::Value* trueStr = builder->CreateGlobalStringPtr("true");
        llvm::Value* falseStr = builder->CreateGlobalStringPtr("false");
        llvm::Value* boolStr = builder->CreateSelect(arg, trueStr, falseStr);
        printfArgs = {formatStr, boolStr};
    }
    else if (auto* ptrTy = llvm::dyn_cast<llvm::PointerType>(arg->getType())) {
        // If it's a pointer to an integer, load it first
        if (ptrTy->getElementType()->isIntegerTy(32)) {
            arg = builder->CreateLoad(ptrTy->getElementType(), arg);
            formatStr = builder->CreateGlobalStringPtr("%d\n");
            printfArgs = {formatStr, arg};
        }
        else {
            reportError("Unsupported pointer type for print()", node->loc);
            return;
        }
    }
    else {
        reportError("Unsupported type for print()", node->loc);
        return;
    }

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

    // Get the type from the argument
    auto* typeExpr = dynamic_cast<TypeExpr*>(node->arguments[0].get());
    if (!typeExpr) {
        reportError("sizeof() argument must be a type", node->loc);
        return nullptr;
    }

    llvm::Type* type = getLLVMType(typeExpr->type);
    if (!type) {
        reportError("Invalid type for sizeof()", node->loc);
        return nullptr;
    }

    // Get size using DataLayout
    const llvm::DataLayout& dataLayout = module->getDataLayout();
    uint64_t size = dataLayout.getTypeAllocSize(type);

    // Return size as i32
    return llvm::ConstantInt::get(context, llvm::APInt(32, size));
}


void CodegenVisitor::generateMallocCall(CallExpr* node) {
    if (node->arguments.size() != 1) {
        reportError("malloc() requires exactly one size argument", node->loc);
        lastValue = nullptr;
        return;
    }

    // Generate code for size argument
    node->arguments[0]->accept(this);
    if (!lastValue) return;

    // Convert size to i64 if needed
    llvm::Value* size = builder->CreateSExtOrTrunc(lastValue, llvm::Type::getInt64Ty(context));

    // Call malloc
    llvm::Function* mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) {
        reportError("malloc function not found", node->loc);
        return;
    }

    // Call malloc to get raw pointer
    llvm::Value* rawPtr = builder->CreateCall(mallocFunc, {size}, "mallocraw");
    
    // Determine the target type for the malloc result
    llvm::Type* targetType = nullptr;
    if (auto* parent = dynamic_cast<VarDeclStmt*>(getCurrentParent(node))) {
        if (parent->type.isArray) {
            targetType = getLLVMType(Type(parent->type.name, false));  // Get base type
            if (targetType) {
                targetType = targetType->getPointerTo();  // Make it a pointer type
            }
        }
    }

    // If we couldn't determine target type, default to i32*
    if (!targetType) {
        targetType = llvm::Type::getInt32PtrTy(context);
    }

    // Cast the raw pointer to the appropriate type
    lastValue = builder->CreateBitCast(rawPtr, targetType, "array_ptr");
}

// Helper function to get parent node
ASTNode* CodegenVisitor::getCurrentParent(ASTNode* node) {
    return node ? node->parent : nullptr;
}

void CodegenVisitor::generateFreeCall(CallExpr* node) {
    if (node->arguments.size() != 1) {
        reportError("free() requires exactly one pointer argument", node->loc);
        lastValue = nullptr;
        return;
    }

    node->arguments[0]->accept(this);
    llvm::Value* ptr = lastValue;
    if (!ptr) return;

    // Cast the pointer to i8* before calling free
    llvm::Value* i8Ptr = builder->CreateBitCast(ptr, llvm::Type::getInt8PtrTy(context), "free.arg");
    llvm::Function* freeFunc = module->getFunction("free");
    lastValue = builder->CreateCall(freeFunc, {i8Ptr});
}

void CodegenVisitor::generateReallocCall(CallExpr* node) {
    if (node->arguments.size() != 2) {
        reportError("realloc() requires exactly two arguments: pointer and size", node->loc);
        lastValue = nullptr;
        return;
    }

    // Generate code for the pointer argument
    node->arguments[0]->accept(this);
    llvm::Value* ptr = lastValue;
    if (!ptr) {
        reportError("Invalid pointer argument for realloc", node->loc);
        return;
    }

    // Generate code for the size argument
    node->arguments[1]->accept(this);
    llvm::Value* size = lastValue;
    if (!size) {
        reportError("Invalid size argument for realloc", node->loc);
        return;
    }

    // Ensure the size is of type i64
    llvm::Value* sizeI64 = builder->CreateZExtOrTrunc(size, llvm::Type::getInt64Ty(context), "size.i64");

    // Cast the pointer to i8* as realloc expects this type
    llvm::Value* castedPtr = builder->CreateBitCast(ptr, llvm::Type::getInt8PtrTy(context), "realloc.ptr");

    // Call realloc
    llvm::Function* reallocFunc = module->getFunction("realloc");
    if (!reallocFunc) {
        reportError("realloc function not found", node->loc);
        lastValue = nullptr;
        return;
    }

    llvm::Value* rawPtr = builder->CreateCall(reallocFunc, {castedPtr, sizeI64}, "realloc.raw");

    // Cast the result back to the appropriate pointer type
    llvm::Type* targetType = ptr->getType();
    lastValue = builder->CreateBitCast(rawPtr, targetType, "realloc.result");
}


void CodegenVisitor::visit(AssignExpr* node) {
    if (!node) return;

    // Handle compound assignments (e.g., +=, -=, *=, /=)
    if (node->op.type != EQUALS) {
        // Process the target as an lvalue
        isAssignmentTarget = true;
        node->target->accept(this);
        isAssignmentTarget = false;

        llvm::Value* targetPtr = lastValue; // Pointer to the target
        if (!targetPtr) {
            reportError("Invalid target for compound assignment", node->loc);
            return;
        }

        // Load the current value of the target
        llvm::Value* currentValue = builder->CreateLoad(
            targetPtr->getType()->getPointerElementType(),
            targetPtr,
            "compound.current"
        );

        // Generate code for the right-hand side expression
        node->value->accept(this);
        llvm::Value* rhsValue = lastValue;
        if (!rhsValue) {
            reportError("Invalid value in compound assignment", node->loc);
            return;
        }

        // Perform the compound operation
        llvm::Value* result = nullptr;
        bool isFloat = currentValue->getType()->isDoubleTy();

        switch (node->op.type) {
            case PLUS_EQUALS:
                result = isFloat ?
                    builder->CreateFAdd(currentValue, rhsValue, "addtmp") :
                    builder->CreateAdd(currentValue, rhsValue, "addtmp");
                break;
            case MINUS_EQUALS:
                result = isFloat ?
                    builder->CreateFSub(currentValue, rhsValue, "subtmp") :
                    builder->CreateSub(currentValue, rhsValue, "subtmp");
                break;
            case STAR_EQUALS:
                result = isFloat ?
                    builder->CreateFMul(currentValue, rhsValue, "multmp") :
                    builder->CreateMul(currentValue, rhsValue, "multmp");
                break;
            case SLASH_EQUALS:
                result = isFloat ?
                    builder->CreateFDiv(currentValue, rhsValue, "divtmp") :
                    builder->CreateSDiv(currentValue, rhsValue, "divtmp");
                break;
            default:
                reportError("Unknown compound assignment operator", node->loc);
                return;
        }

        // Store the result back into the target
        builder->CreateStore(result, targetPtr);
        lastValue = result;
        return;
    }

    // Handle regular assignment (=)
    node->value->accept(this);
    llvm::Value* value = lastValue;
    if (!value) {
        reportError("Invalid value in assignment", node->loc);
        return;
    }

    if (auto* var = dynamic_cast<VariableExpr*>(node->target.get())) {
        // Handle variable assignment
        Symbol* symbol = symbolTable.resolve(var->name.value);
        if (!symbol || !symbol->llvmValue) {
            reportError("Undefined variable: " + var->name.value, node->loc);
            return;
        }

        // Convert value type if needed
        llvm::Type* targetType = getLLVMType(symbol->type);
        if (value->getType() != targetType) {
            value = convertValue(value, targetType);
            if (!value) {
                reportError("Invalid type conversion in assignment", node->loc);
                return;
            }
        }

        builder->CreateStore(value, symbol->llvmValue);
        lastValue = value;
    } else if (auto* arrayAccess = dynamic_cast<ArrayAccessExpr*>(node->target.get())) {
        // Handle array element assignment
        isAssignmentTarget = true;
        arrayAccess->accept(this);
        isAssignmentTarget = false;

        llvm::Value* targetPtr = lastValue; // Pointer to array element
        if (!targetPtr) {
            reportError("Invalid array access in assignment", node->loc);
            return;
        }

        // Convert value type to match array element type
        llvm::Type* elementType = targetPtr->getType()->getPointerElementType();
        if (value->getType() != elementType) {
            value = convertValue(value, elementType);
            if (!value) {
                reportError("Invalid type conversion in array assignment", node->loc);
                return;
            }
        }

        builder->CreateStore(value, targetPtr);
        lastValue = value;
    } else {
        reportError("Invalid assignment target", node->loc);
        lastValue = nullptr;
    }
}


void CodegenVisitor::visit(ArrayInitExpr* node) {
    if (node->elements.empty()) {
        lastValue = nullptr;
        return;
    }

    // Get type from first element
    node->elements[0]->accept(this);
    if (!lastValue) {
        reportError("Invalid first element in array initializer", node->loc);
        return;
    }
    llvm::Type* elementType = lastValue->getType();

    // Create array type
    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, 
                                                     node->elements.size());
    
    // Create allocation for the array
    llvm::Value* arrayAlloca = builder->CreateAlloca(arrayType, 
                                                    nullptr, 
                                                    "arrayinit");

    // Store each element
    for (size_t i = 0; i < node->elements.size(); i++) {
        node->elements[i]->accept(this);
        if (!lastValue) continue;

        // Convert element to correct type if needed
        lastValue = convertValue(lastValue, elementType);

        // Create GEP for element storage
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, i))
        };
        llvm::Value* elementPtr = builder->CreateInBoundsGEP(arrayType, 
                                                           arrayAlloca, 
                                                           indices, 
                                                           "arrayelem");
        
        // Store the element
        builder->CreateStore(lastValue, elementPtr);
    }

    lastValue = arrayAlloca;
}

void CodegenVisitor::visit(ArrayAllocExpr* node) {
    // Generate code for size expression
    node->size->accept(this);
    llvm::Value* size = lastValue;
    
    if (!size) {
        reportError("Invalid array size expression", node->loc);
        return;
    }

    // Ensure size is i32
    if (!size->getType()->isIntegerTy(32)) {
        size = builder->CreateIntCast(size, 
                                    llvm::Type::getInt32Ty(context), 
                                    true, 
                                    "sizecast");
    }

    // Get element type
    llvm::Type* elementType = getLLVMType(node->elementType);
    if (!elementType) {
        reportError("Invalid array element type", node->loc);
        return;
    }

    // Calculate element size using DataLayout
    const llvm::DataLayout& dataLayout = module->getDataLayout();
    uint64_t elemSize = dataLayout.getTypeAllocSize(elementType);
    llvm::Value* elemSizeVal = llvm::ConstantInt::get(context, 
                                                     llvm::APInt(64, elemSize));

    // Calculate total size (size * sizeof(element))
    llvm::Value* totalSize = builder->CreateMul(
        builder->CreateZExt(size, llvm::Type::getInt64Ty(context)),
        elemSizeVal,
        "totalsize"
    );

    // Call malloc
    llvm::Function* mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) {
        reportError("malloc function not found", node->loc);
        return;
    }

    llvm::Value* memory = builder->CreateCall(mallocFunc, {totalSize}, "mallocraw");

    // Bitcast malloc result to array element type pointer
    lastValue = builder->CreateBitCast(memory, 
                                     elementType->getPointerTo(), 
                                     "arrayptr");
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
    llvm::Type* varType = getLLVMType(node->type);
    if (!varType) {
        reportError("Invalid type for variable: " + node->name.value, node->loc);
        return;
    }

    // Create the alloca instruction at the beginning of the function
    llvm::Value* alloca = nullptr;
    if (currentFunction) {
        alloca = generateAlloca(currentFunction, node->name.value, varType);
    } else {
        reportError("Variable declaration outside function", node->loc);
        return;
    }

    // Handle initialization
    if (node->initializer) {
        // Handle array initialization
        if (node->type.isArray) {
            if (auto* arrayInit = dynamic_cast<ArrayInitExpr*>(node->initializer.get())) {
                // Fixed-size array initialization
                if (node->type.arraySize >= 0) {
                    size_t numElements = std::min(
                        arrayInit->elements.size(),
                        static_cast<size_t>(node->type.arraySize)
                    );

                    // Initialize declared elements
                    for (size_t i = 0; i < numElements; i++) {
                        // Create indices for element access
                        std::vector<llvm::Value*> indices = {
                            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                            llvm::ConstantInt::get(context, llvm::APInt(32, i))
                        };

                        // Get pointer to array element
                        llvm::Value* elementPtr = builder->CreateInBoundsGEP(
                            varType,
                            alloca,
                            indices,
                            "array.element"
                        );

                        // Generate code for the initializer value
                        arrayInit->elements[i]->accept(this);
                        if (!lastValue) continue;

                        // Convert type if needed
                        llvm::Type* elementType = elementPtr->getType()->getPointerElementType();
                        lastValue = convertValue(lastValue, elementType);
                        
                        // Store the value
                        builder->CreateStore(lastValue, elementPtr);
                    }

                    // Zero-initialize remaining elements
                    if (numElements < static_cast<size_t>(node->type.arraySize)) {
                        llvm::Type* elementType = varType->getArrayElementType();
                        llvm::Value* zero = llvm::Constant::getNullValue(elementType);
                        
                        for (size_t i = numElements; i < static_cast<size_t>(node->type.arraySize); i++) {
                            std::vector<llvm::Value*> indices = {
                                llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                                llvm::ConstantInt::get(context, llvm::APInt(32, i))
                            };
                            
                            llvm::Value* elementPtr = builder->CreateInBoundsGEP(
                                varType,
                                alloca,
                                indices,
                                "array.element"
                            );
                            
                            builder->CreateStore(zero, elementPtr);
                        }
                    }
                }
                // Dynamic array initialization
                else {
                    // Handle malloc or other dynamic initialization
                    node->initializer->accept(this);
                    if (lastValue) {
                        builder->CreateStore(lastValue, alloca);
                    }
                }
            }
            // Handle malloc call for dynamic arrays
            else if (auto* callExpr = dynamic_cast<CallExpr*>(node->initializer.get())) {
                if (callExpr->name.value == "malloc") {
                    node->initializer->accept(this);
                    if (lastValue) {
                        // Store the malloc result
                        builder->CreateStore(lastValue, alloca);
                    }
                }
            }
        }
        // Handle non-array initialization
        else {
            node->initializer->accept(this);
            if (lastValue) {
                // Convert the initializer value to the variable's type if needed
                lastValue = convertValue(lastValue, varType);
                if (lastValue) {
                    builder->CreateStore(lastValue, alloca);
                }
            }
        }
    }
    // No initializer - zero initialize
    else {
        if (node->type.isArray && node->type.arraySize >= 0) {
            // Zero initialize fixed-size array
            builder->CreateStore(
                llvm::ConstantAggregateZero::get(varType),
                alloca
            );
        }
        else if (node->type.isArray) {
            // Initialize dynamic array pointer to null
            builder->CreateStore(
                llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(varType)),
                alloca
            );
        }
        else {
            // Zero initialize primitive type
            builder->CreateStore(
                llvm::Constant::getNullValue(varType),
                alloca
            );
        }
    }

    // Update symbol table
    symbolTable.declare(node->name.value, node->type);
    if (Symbol* symbol = symbolTable.resolve(node->name.value)) {
        symbol->llvmValue = alloca;
        symbol->isAlloca = true;
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