#include <string>
#include <iostream>

#include "ast.h"

FunctionAST::FunctionAST(const std::string &name, 
                         const std::string &returnType, 
                         std::unique_ptr<ASTNode> body)
    : name(name), returnType(returnType), body(std::move(body)) {}

ReturnAST::ReturnAST(int v) : value(v) {}
ReturnAST::ReturnAST(float f) : value(f) {}
ReturnAST::ReturnAST(const std::string &s) : value(s) {}