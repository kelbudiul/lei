// src/compiler.cpp
#include "compiler.h"

Compiler::Compiler(const std::string& sourceCode)
    : input(sourceCode), 
      lexer(sourceCode)
      // parser({}) // Will be initialized during compilation
      {}

std::vector<Token> Compiler::lexicalAnalysis() {
    try {
        return lexer.tokenize();
    } catch (const std::exception& e) {
        reportError("Lexical Analysis", e.what());
        throw;
    }
}

std::unique_ptr<ASTNode> Compiler::syntaxAnalysis(const std::vector<Token>& tokens) {
    Parser localParser(tokens);
    try {
        return localParser.parse();
    } catch (const std::exception& e) {
        reportError("Syntax Analysis", e.what());
        throw;
    }
}

void Compiler::semanticAnalysis(const ASTNode* ast) {
    // Basic semantic checks
    if (!ast) {
        reportError("Semantic Analysis", "Empty abstract syntax tree");
        return;
    }
    
    // Future: Add more sophisticated semantic checks
    // - Type checking
    // - Scope analysis
    // - Constant folding
}

void Compiler::generateCode(const ASTNode* ast) {
    CodeGenerator codeGen;
    try {
        codeGen.generateIR(ast);
    } catch (const std::exception& e) {
        reportError("Code Generation", e.what());
        throw;
    }
}

int Compiler::compile() {
    // Compilation pipeline
    auto tokens = lexicalAnalysis();
    auto ast = syntaxAnalysis(tokens);
    
    semanticAnalysis(ast.get());
    generateCode(ast.get());
    
    // For now, just return the evaluated result
    return ast->evaluate();
}

void Compiler::reportError(const std::string& stage, const std::string& message) {
    std::cerr << "Compilation Error in " << stage << ": " << message << std::endl;
}