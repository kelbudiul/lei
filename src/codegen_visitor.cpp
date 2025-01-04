#define CHECK_NODE(node, message) \
    if (!node) { \
        reportError(message, Location()); \
        lastValue = nullptr; \
        return; \
    }


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
      lastValue(nullptr),
      typeHelper(TypeHelper::instance(&ctx, builder.get())) {}
      
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

llvm::Value* CodegenVisitor::generateAlloca(llvm::Function* function, const std::string& name, llvm::Type* type) {
    // Create IRBuilder for the entry block
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(), 
                                function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}


void CodegenVisitor::visit(Program* node) {
    if (!node) {
        reportError("Null program node", Location());
        return;
    }


    // First pass: declare all functions
    for (const auto& func : node->functions) {
        if (!func) continue;
        
        
        auto symbol = symbolTable.resolveFunction(func->name.value);
        if (!symbol) {
            reportError("Function symbol not found: " + func->name.value, func->loc);
            continue;
        }

        // Create function type
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : func->parameters) {
            if (auto paramType = typeHelper.getLLVMType(param.type)) {
                paramTypes.push_back(paramType);
            } else {
                reportError("Invalid parameter type in function: " + func->name.value, func->loc);
                return;
            }
        }

        llvm::Type* returnType = typeHelper.getLLVMType(func->returnType);
        if (!returnType) {
            reportError("Invalid return type for function: " + func->name.value, func->loc);
            return;
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
        llvm::Function* function = llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, func->name.value, module.get()
        );

        // Store LLVM function in symbol table
        symbol->llvmFunction = function;
    }


    // Second pass: generate function bodies

    for (const auto& func : node->functions) {
        if (!func) continue;
        func->accept(this);
    }
}
void CodegenVisitor::visit(FunctionDecl* node) {
    // Get or create the function
    llvm::Function* function = module->getFunction(node->name.value);
    if (!function) {
        // Create function type with proper array parameter handling
        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : node->parameters) {
            llvm::Type* paramType = typeHelper.getLLVMType(param.type);
            if (!paramType) {
                reportError("Invalid parameter type", Location(param.name));
                return;
            }
            
            // Convert array parameters to pointers
            if (param.type.isArray) {
                if (auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(paramType)) {
                    paramType = arrayType->getElementType()->getPointerTo();
                } else if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(paramType)) {
                    // Already a pointer type (for dynamic arrays)
                    paramType = ptrType;
                }
            }
            paramTypes.push_back(paramType);
        }

        llvm::Type* returnType = typeHelper.getLLVMType(node->returnType);
        if (!returnType) {
            reportError("Invalid return type", node->loc);
            return;
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
        function = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            node->name.value,
            module.get()
        );
    }

    // Set current function and create entry block
    currentFunction = function;
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", function);
    builder->SetInsertPoint(entry);

    // Handle parameters
    symbolTable.enterScope();
    auto argIt = function->arg_begin();
    for (const auto& param : node->parameters) {
        llvm::Type* paramType = typeHelper.getLLVMType(param.type);
        if (!paramType) {
            reportError("Invalid parameter type", Location(param.name));
            return;
        }

        // Create alloca for parameter
        llvm::Value* alloca = generateAlloca(function, param.name.value, paramType);
        
        // Store the parameter value
        builder->CreateStore(&*argIt, alloca);

        // Update symbol table
        if (!symbolTable.declare(param.name.value, param.type)) {
            reportError("Parameter redeclaration: " + param.name.value, node->loc);
            return;
        }
        
        auto symbol = symbolTable.resolve(param.name.value);
        if (symbol) {
            symbol->llvmValue = alloca;
            symbol->isAlloca = true;
        }

        ++argIt;
    }

    // Generate function body
    node->body->accept(this);

    // Add return if needed
    if (!builder->GetInsertBlock()->getTerminator()) {
        if (node->returnType.name == "void") {
            builder->CreateRetVoid();
        } else {
            builder->CreateRet(llvm::Constant::getNullValue(typeHelper.getLLVMType(node->returnType)));
        }
    }

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

    // For arrays, return the address directly without loading
    llvm::Type* type = symbol->llvmValue->getType()->getPointerElementType();
    if (type->isArrayTy() || (type->isPointerTy() && !isAssignmentTarget)) {
        lastValue = symbol->llvmValue;
        return;
    }

    // For other types, load the value if not an assignment target
    if (!isAssignmentTarget) {
        lastValue = builder->CreateLoad(type, symbol->llvmValue);
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
        left = typeHelper.convert(left, llvm::Type::getDoubleTy(context));
        right = typeHelper.convert(right, llvm::Type::getDoubleTy(context));
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
    // Generate code for the array base
    node->array->accept(this);
    llvm::Value* arrayBase = lastValue;

    // Generate code for the index
    node->index->accept(this);
    llvm::Value* index = lastValue;

    if (!arrayBase || !index) {
        reportError("Invalid array access", node->loc);
        lastValue = nullptr;
        return;
    }

    // If the index is a pointer (e.g., from a variable), load its value first
    if (index->getType()->isPointerTy()) {
        index = builder->CreateLoad(
            index->getType()->getPointerElementType(),
            index,
            "index.load"
        );
    }

    // Now ensure the index is i32
    if (!index->getType()->isIntegerTy(32)) {
        index = builder->CreateIntCast(
            index,
            llvm::Type::getInt32Ty(context),
            true,  // isSigned
            "index.cast"
        );
    }

    llvm::Value* elementPtr = nullptr;
    
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayBase)) {
        llvm::Type* allocatedType = allocaInst->getAllocatedType();
        
        if (allocatedType->isArrayTy()) {
            // Static array case
            llvm::Value* zero = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            elementPtr = builder->CreateInBoundsGEP(
                allocatedType,
                arrayBase,
                {zero, index},
                "static.array.element"
            );
        } 
        else if (allocatedType->isPointerTy()) {
            // Dynamic array case
            llvm::Value* arrayPtr = builder->CreateLoad(
                allocatedType,
                arrayBase,
                "dynamic.array.ptr"
            );
            elementPtr = builder->CreateInBoundsGEP(
                allocatedType->getPointerElementType(),
                arrayPtr,
                index,
                "dynamic.array.element"
            );
        }
    }

    if (!elementPtr) {
        reportError("Invalid array access", node->loc);
        lastValue = nullptr;
        return;
    }

    // For assignments, return the pointer to the element
    if (isAssignmentTarget) {
        lastValue = elementPtr;
        return;
    }

    // For reads, load the value from the element pointer
    lastValue = builder->CreateLoad(
        elementPtr->getType()->getPointerElementType(),
        elementPtr,
        "array.load"
    );
}


void CodegenVisitor::declareRuntimeFunctions() {
    // Declare common types
    llvm::Type* int32Ty = llvm::Type::getInt32Ty(context);
    llvm::Type* int64Ty = llvm::Type::getInt64Ty(context);
    llvm::Type* voidTy = llvm::Type::getVoidTy(context);
    llvm::Type* int8PtrTy = llvm::Type::getInt8PtrTy(context);
    llvm::Type* doubleTy = llvm::Type::getDoubleTy(context);

    // Declare runtime functions using the helper
    declareFunction("printf", int32Ty, {int8PtrTy}, true);
    declareFunction("malloc", int8PtrTy, {int64Ty});
    declareFunction("free", voidTy, {int8PtrTy});
    declareFunction("realloc", int8PtrTy, {int8PtrTy, int64Ty});
    declareFunction("strlen", int64Ty, {int8PtrTy});
    declareFunction("strcmp", llvm::Type::getInt32Ty(context), {llvm::Type::getInt8PtrTy(context), llvm::Type::getInt8PtrTy(context)});
    declareFunction("strcpy", llvm::Type::getInt8PtrTy(context), {llvm::Type::getInt8PtrTy(context), llvm::Type::getInt8PtrTy(context)});
    declareFunction("strcat", llvm::Type::getInt8PtrTy(context), {llvm::Type::getInt8PtrTy(context), llvm::Type::getInt8PtrTy(context)});
    declareFunction("pow", llvm::Type::getDoubleTy(context), {llvm::Type::getDoubleTy(context), llvm::Type::getDoubleTy(context)});
    declareFunction("sqrt", llvm::Type::getDoubleTy(context), {llvm::Type::getDoubleTy(context)});
    declareFunction("toupper", llvm::Type::getInt32Ty(context), {llvm::Type::getInt32Ty(context)});
    declareFunction("tolower", llvm::Type::getInt32Ty(context), {llvm::Type::getInt32Ty(context)});
    declareFunction("atoi", llvm::Type::getInt32Ty(context), {llvm::Type::getInt8PtrTy(context)});
    declareFunction("atof", llvm::Type::getDoubleTy(context), {llvm::Type::getInt8PtrTy(context)});
    declareFunction("itoa", llvm::Type::getInt8PtrTy(context), {llvm::Type::getInt32Ty(context), llvm::Type::getInt8PtrTy(context), llvm::Type::getInt32Ty(context)});

    llvm::Type* filePtr = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
    module->getOrInsertGlobal("stdin", filePtr);

    declareFunction("fgets", llvm::Type::getInt8PtrTy(context),{
    llvm::Type::getInt8PtrTy(context),  // buffer
    llvm::Type::getInt32Ty(context),    // size
    filePtr                             // stream
        }
    );

}

void CodegenVisitor::visit(CallExpr* node) {
    if (!node) {
        reportError("Null call expression", Location());
        return;
    }

    // First check if this is a built-in function
    lastValue = handleBuiltinFunction(node);
    if (lastValue) {
        return;  // It was a built-in function and was handled successfully
    }

    // Handle regular function call
    lastValue = handleRegularFunctionCall(node);
}



llvm::Value* CodegenVisitor::handleBuiltinFunction(CallExpr* node) {
    // A simpler approach to handle built-in functions
    if (node->name.value == "print") {
        generatePrintCall(node);
        return lastValue;
    }
    if (node->name.value == "input") {
        generateInputCall(node);
        return lastValue;
    }
    if (node->name.value == "malloc") {
        generateMallocCall(node);
        return lastValue;
    }
    if (node->name.value == "free") {
        generateFreeCall(node);
        return lastValue;
    }
    if (node->name.value == "realloc") {
        generateReallocCall(node);
        return lastValue;
    }
    if (node->name.value == "strlen") {
        generateStrlenCall(node);
        return lastValue;
    }
    if (node->name.value == "sizeof") {
        return generateSizeofCall(node);
    }

    // Handle conversion functions (atoi, atof, itoa, ftoa)
    if (node->name.value == "atoi" || node->name.value == "atof" ||
        node->name.value == "itoa" || node->name.value == "ftoa") {
        
        if (node->arguments.size() != 1) {
            reportError("Function " + node->name.value + " expects one argument", node->loc);
            return nullptr;
        }

        node->arguments[0]->accept(this);
        if (!lastValue) {
            return nullptr;
        }

        llvm::Function* func = module->getFunction(node->name.value);
        return builder->CreateCall(func, {lastValue}, "convert.tmp");
    }

    return nullptr;
}

llvm::Value* CodegenVisitor::handleRegularFunctionCall(CallExpr* node) {
    // Look up the function in symbol table
    auto* funcSymbol = symbolTable.resolveFunction(node->name.value);
    if (!funcSymbol || !funcSymbol->llvmFunction) {
        reportError("Undefined function: " + node->name.value, node->loc);
        return nullptr;
    }

    // Process arguments and create call
    std::vector<llvm::Value*> processedArgs = processCallArguments(node, funcSymbol);
    if (processedArgs.empty() && !node->arguments.empty()) {
        // Error already reported in processCallArguments
        return nullptr;
    }

    return builder->CreateCall(funcSymbol->llvmFunction, processedArgs);
}

std::vector<llvm::Value*> CodegenVisitor::processCallArguments(
    CallExpr* node, FunctionSymbol* funcSymbol) {
    
    std::vector<llvm::Value*> args;
    
    // Validate argument count
    if (node->arguments.size() != funcSymbol->parameters.size()) {
        reportError("Wrong number of arguments for function " + node->name.value +
                   ". Expected " + std::to_string(funcSymbol->parameters.size()) +
                   " but got " + std::to_string(node->arguments.size()),
                   node->loc);
        return args;
    }

    // Process each argument
    for (size_t i = 0; i < node->arguments.size(); i++) {
        node->arguments[i]->accept(this);
        llvm::Value* arg = lastValue;
        
        if (!arg) continue;

        // Handle array arguments specially
        if (i < funcSymbol->parameters.size() && 
            funcSymbol->parameters[i].type.isArray) {
            
            arg = handleArrayArgument(arg);
        }
        
        // Convert types if needed
        llvm::Type* paramType = typeHelper.getLLVMType(funcSymbol->parameters[i].type);
        if (arg->getType() != paramType) {
            arg = typeHelper.convert(arg, paramType);
        }

        args.push_back(arg);
    }

    return args;
}

llvm::Value* CodegenVisitor::handleArrayArgument(llvm::Value* arg) {
    // Handle array argument conversions
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arg)) {
        // For dynamic arrays (pointer type alloca), load the pointer
        if (allocaInst->getAllocatedType()->isPointerTy()) {
            arg = builder->CreateLoad(
                allocaInst->getAllocatedType(),
                arg,
                "array.arg"
            );
        } else if (allocaInst->getAllocatedType()->isArrayTy()) {
            // For fixed arrays, get pointer to first element
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                llvm::ConstantInt::get(context, llvm::APInt(32, 0))
            };
            arg = builder->CreateInBoundsGEP(
                allocaInst->getAllocatedType(),
                arg,
                indices,
                "array.arg"
            );
        }
    }
    return arg;
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
    // Validate that print() has at least one argument
    if (node->arguments.empty()) {
        reportError("print() requires an argument", node->loc);
        lastValue = nullptr;
        return;
    }

    // Get printf function declaration from the module
    llvm::Function* printfFunc = module->getFunction("printf");
    if (!printfFunc) {
        reportError("printf function not found", node->loc);
        return;
    }

    // Generate code for the argument to be printed
    node->arguments[0]->accept(this);
    llvm::Value* arg = lastValue;
    if (!arg) return;

    // If the argument is a loaded value, keep track of it
    if (auto* loadInst = llvm::dyn_cast<llvm::LoadInst>(arg)) {
        arg = loadInst;
    }

    std::vector<llvm::Value*> printfArgs;
    llvm::Value* formatStr = nullptr;

    // Handle different types with appropriate format strings and conversions
    if (arg->getType()->isIntegerTy(32)) {
        // Integer type (32-bit)
        formatStr = builder->CreateGlobalStringPtr("%d");
        printfArgs = {formatStr, arg};
    } 
    else if (arg->getType()->isDoubleTy()) {
        // Floating point type
        formatStr = builder->CreateGlobalStringPtr("%f");
        printfArgs = {formatStr, arg};
    }
    else if (arg->getType()->isPointerTy() && 
             arg->getType()->getPointerElementType()->isIntegerTy(8)) {
        // Direct string pointer (string literals)
        formatStr = builder->CreateGlobalStringPtr("%s");
        printfArgs = {formatStr, arg};
    }
    else if (arg->getType()->isPointerTy() && 
             arg->getType()->getPointerElementType()->isPointerTy() && 
             arg->getType()->getPointerElementType()->getPointerElementType()->isIntegerTy(8)) {
        // Pointer to string pointer (string variables)
        formatStr = builder->CreateGlobalStringPtr("%s");
        // Load the actual string pointer from the string variable
        llvm::Value* strPtr = builder->CreateLoad(
            arg->getType()->getPointerElementType(),
            arg,
            "str.ptr"
        );
        printfArgs = {formatStr, strPtr};
    }
    else if (arg->getType()->isIntegerTy(1)) {
        // Boolean type - convert to "true" or "false" string
        formatStr = builder->CreateGlobalStringPtr("%s");
        llvm::Value* trueStr = builder->CreateGlobalStringPtr("true");
        llvm::Value* falseStr = builder->CreateGlobalStringPtr("false");
        llvm::Value* boolStr = builder->CreateSelect(arg, trueStr, falseStr);
        printfArgs = {formatStr, boolStr};
    }
    else if (auto* ptrTy = llvm::dyn_cast<llvm::PointerType>(arg->getType())) {
        // Handle pointers to integers - need to load the value first
        if (ptrTy->getElementType()->isIntegerTy(32)) {
            arg = builder->CreateLoad(ptrTy->getElementType(), arg);
            formatStr = builder->CreateGlobalStringPtr("%d");
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

    // Create the actual call to printf
    lastValue = builder->CreateCall(printfFunc, printfArgs);
}


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

    llvm::Type* type = typeHelper.getLLVMType(typeExpr->type);
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

    // Convert size to i64
    llvm::Value* size = builder->CreateSExtOrTrunc(
        lastValue, 
        llvm::Type::getInt64Ty(context),
        "malloc.size"
    );

    // Call malloc
    llvm::Function* mallocFunc = module->getFunction("malloc");
    if (!mallocFunc) {
        reportError("malloc function not found", node->loc);
        return;
    }

    // Get the target element type from the context
    llvm::Type* targetType = nullptr;
    if (auto* parent = dynamic_cast<VarDeclStmt*>(node->parent)) {
        // Get base type without array modifier
        targetType = typeHelper.getLLVMType(Type(parent->type.name, false));
    }
    
    if (!targetType) {
        targetType = llvm::Type::getInt8Ty(context); // Default to char
    }

    // Call malloc and get raw pointer
    llvm::Value* rawPtr = builder->CreateCall(mallocFunc, {size}, "malloc.raw");

    // Cast to the correct target pointer type
    lastValue = builder->CreateBitCast(
        rawPtr,
        targetType->getPointerTo(),
        "malloc.typed"
    );
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

    // If we have a pointer to a pointer (e.g., i32**), load the actual pointer first
    if (auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
        if (allocaInst->getAllocatedType()->isPointerTy()) {
            ptr = builder->CreateLoad(allocaInst->getAllocatedType(), ptr, "free.ptr");
        }
    }

    // Cast the pointer to i8* for free
    llvm::Value* i8Ptr = builder->CreateBitCast(ptr, llvm::Type::getInt8PtrTy(context), "free.i8ptr");
    
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
        llvm::Type* targetType = typeHelper.getLLVMType(symbol->type);
        if (value->getType() != targetType) {
            value = typeHelper.convert(value, targetType);
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
            value = typeHelper.convert(value, elementType);
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
        lastValue = typeHelper.convert(lastValue, elementType);

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
    llvm::Type* elementType = typeHelper.getLLVMType(node->elementType);
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


// In codegen_visitor.cpp

void CodegenVisitor::visit(VarDeclStmt* node) {
    // Step 1: Create the allocation
    llvm::Value* alloca = createVariableAllocation(node);
    if (!alloca) return;

    // Step 2: Handle initialization
    handleVariableInitialization(node, alloca);

    // Step 3: Update symbol table
    updateSymbolTableEntry(node->name.value, node->type, alloca);
}

llvm::Value* CodegenVisitor::createVariableAllocation(VarDeclStmt* node) {
    if (!currentFunction) {
        reportError("Variable declaration outside function", node->loc);
        return nullptr;
    }

    // Get the base type without array modifier
    Type baseType(node->type.name, false);
    llvm::Type* elementType = typeHelper.getLLVMType(baseType);
    if (!elementType) {
        reportError("Invalid type for variable: " + node->name.value, node->loc);
        return nullptr;
    }

    llvm::Type* allocType;
    if (node->type.isArray) {
        if (node->type.arraySize >= 0) {
            // Fixed-size array
            allocType = llvm::ArrayType::get(elementType, node->type.arraySize);
        } else {
            // Dynamic array - allocate space for the pointer
            allocType = elementType->getPointerTo();
        }
    } else {
        // Regular variable
        allocType = elementType;
    }

    return generateAlloca(currentFunction, node->name.value, allocType);
}

void CodegenVisitor::handleVariableInitialization(VarDeclStmt* node, llvm::Value* alloca) {
    if (!node->initializer) {
        // Handle default initialization
        llvm::Type* varType = alloca->getType()->getPointerElementType();
        if (node->type.isArray && node->type.arraySize >= 0) {
            // Fixed-size array initialization
            builder->CreateStore(llvm::ConstantAggregateZero::get(varType), alloca);
        } else if (node->type.isArray) {
            // Dynamic array initialization
            builder->CreateStore(
                llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(varType)),
                alloca
            );
        } else {
            // Scalar initialization
            builder->CreateStore(llvm::Constant::getNullValue(varType), alloca);
        }
        return;
    }

    // Handle initialization with a value
    if (node->type.isArray) {
        if (auto* arrayInit = dynamic_cast<ArrayInitExpr*>(node->initializer.get())) {
            if (node->type.arraySize >= 0) {
                initializeFixedArray(node, alloca, arrayInit);
            } else {
                initializeDynamicArray(node, alloca);
            }
        } else if (auto* callExpr = dynamic_cast<CallExpr*>(node->initializer.get())) {
            // Handle malloc/realloc initialization
            if (callExpr->name.value == "malloc" || callExpr->name.value == "realloc") {
                node->initializer->accept(this);
                if (lastValue) {
                    // Ensure type compatibility
                    llvm::Type* allocaType = alloca->getType()->getPointerElementType();
                    if (lastValue->getType() != allocaType) {
                        lastValue = builder->CreateBitCast(lastValue, allocaType);
                    }
                    builder->CreateStore(lastValue, alloca);
                }
            }
        }
    } else {
        // Handle scalar initialization
        node->initializer->accept(this);
        if (lastValue) {
            llvm::Type* varType = alloca->getType()->getPointerElementType();
            lastValue = typeHelper.convert(lastValue, varType);
            if (lastValue) {
                builder->CreateStore(lastValue, alloca);
            }
        }
    }
}

void CodegenVisitor::initializeFixedArray(VarDeclStmt* node, llvm::Value* alloca, 
                                        ArrayInitExpr* arrayInit) {
    size_t numElements = std::min(
        arrayInit->elements.size(),
        static_cast<size_t>(node->type.arraySize)
    );

    llvm::Type* varType = alloca->getType()->getPointerElementType();
    llvm::Type* elementType = varType->getArrayElementType();

    // Initialize declared elements
    for (size_t i = 0; i < numElements; i++) {
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
            llvm::ConstantInt::get(context, llvm::APInt(32, i))
        };

        llvm::Value* elementPtr = builder->CreateInBoundsGEP(
            varType, alloca, indices, "array.element"
        );

        arrayInit->elements[i]->accept(this);
        if (lastValue) {
            lastValue = typeHelper.convert(lastValue, elementType);
            builder->CreateStore(lastValue, elementPtr);
        }
    }

    // Zero-initialize remaining elements
    if (numElements < static_cast<size_t>(node->type.arraySize)) {
        llvm::Value* zero = llvm::Constant::getNullValue(elementType);
        for (size_t i = numElements; i < static_cast<size_t>(node->type.arraySize); i++) {
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(context, llvm::APInt(32, 0)),
                llvm::ConstantInt::get(context, llvm::APInt(32, i))
            };
            
            llvm::Value* elementPtr = builder->CreateInBoundsGEP(
                varType, alloca, indices, "array.element"
            );
            
            builder->CreateStore(zero, elementPtr);
        }
    }
}

void CodegenVisitor::initializeDynamicArray(VarDeclStmt* node, llvm::Value* alloca) {
    // Generate the initializer value (e.g., malloc call)
    node->initializer->accept(this);
    if (!lastValue) return;

    // The alloca is already a pointer variable (i32**), and lastValue is our malloc'd pointer (i32*)
    // We just need to store the malloc'd pointer in our variable
    builder->CreateStore(lastValue, alloca);
}

void CodegenVisitor::updateSymbolTableEntry(const std::string& name, const Type& type, 
                                          llvm::Value* alloca) {
    symbolTable.declare(name, type);
    if (Symbol* symbol = symbolTable.resolve(name)) {
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
    lastValue = typeHelper.convert(lastValue, returnType);
    
    builder->CreateRet(lastValue);
}

void CodegenVisitor::declareFunction(const std::string& name, llvm::Type* returnType, 
                                     const std::vector<llvm::Type*>& paramTypes, bool isVarArgs) {
    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, isVarArgs);
    module->getOrInsertFunction(name, funcType);
}