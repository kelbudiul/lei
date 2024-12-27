#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include"ast_printer.h"
#include "semantic_visitor.h"
#include "codegen_visitor.h"
#include "source_reader.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

bool Compiler::compile(const std::string& source, const std::string& outputPath) {
    // Lexical Analysis
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (errorHandler.hasErrors()) {
        return false;
    }

    // Parsing
    Parser parser(tokens);
    auto ast = parser.parse();
    if (!ast || errorHandler.hasErrors()) {
        return false;
    }


    // Semantic Analysis
    SemanticAnalyzer analyzer(symbolTable);
    if (!analyzer.analyze(ast.get())) {
        return false;
    }

    // Code Generation
    CodegenVisitor codegen(llvmContext);
    auto module = codegen.generateModule(ast.get(), "module");
    if (!module || errorHandler.hasErrors()) {
        return false;
    }

    // Write output
    std::error_code EC;
    llvm::raw_fd_ostream dest(outputPath, EC, llvm::sys::fs::OF_None);
    if (EC) {
        errorHandler.error(
            ErrorLevel::CODEGEN,
            0, 0,
            "Could not open output file: " + EC.message()
        );
        return false;
    }

    module->print(dest, nullptr);
    return true;
}

bool Compiler::execute(const std::string& source) {
    // Lexical Analysis
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    if (errorHandler.hasErrors()) {
        return false;
    }

    // Parsing
    Parser parser(tokens);
    auto ast = parser.parse();
    if (!ast || errorHandler.hasErrors()) {
        return false;
    }



    // Semantic Analysis
    SemanticAnalyzer analyzer(symbolTable);
    if (!analyzer.analyze(ast.get())) {
        return false;
    }


    symbolTable.print();

    ASTPrinter printer;
    std::cout << "AST Structure:\n" << printer.print(ast.get()) << std::endl;

    // Code Generation
    CodegenVisitor codegen(llvmContext);
    auto module = codegen.generateModule(ast.get(), "module");
    if (!module || errorHandler.hasErrors()) {
        return false;
    }

    symbolTable.print();
    std::cout << "Debug: Functions in module:" << std::endl;
    for (auto& function : module->getFunctionList()) {
        std::cout << function.getName().str() << std::endl;
    }
    // Print module state before execution
    std::cout << "Debug: Module IR before execution:\n";
    module->print(llvm::outs(), nullptr);
    
    // Initialize JIT ExecutionEngine
    std::string errorStr;
    llvm::EngineBuilder engineBuilder(std::move(module));
    auto engine = engineBuilder
        .setErrorStr(&errorStr)
        .setEngineKind(llvm::EngineKind::JIT)
        .create();

    if (!engine) {
        errorHandler.error(ErrorLevel::CODEGEN, 0, 0, 
            "Failed to create execution engine: " + errorStr);
        return false;
    }

    std::cout << "Debug: Looking for main function\n";
    auto mainFunction = engine->FindFunctionNamed("main");
    
    if (!mainFunction) {
        std::cout << "Debug: Function lookup failed\n";
        errorHandler.error(ErrorLevel::CODEGEN, 0, 0, "Failed to find main function in module");
        return false;
    }

    std::cout << "Debug: Found main function at address: " 
              << mainFunction << "\n";

    // Execute the function
    std::vector<llvm::GenericValue> args;
    llvm::GenericValue result = engine->runFunction(mainFunction, args);

    // Print the result
    std::cout << "Execution Result: " << result.IntVal.getSExtValue() << std::endl;

    delete engine;
    return true;
}