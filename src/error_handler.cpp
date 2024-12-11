#include "error_handler.h"
#include <iostream>
#include <sstream>
#include <algorithm>

std::string ErrorHandler::getLevelString(ErrorLevel level) {
    switch (level) {
        case ErrorLevel::LEXICAL:  return "Lexical Error";
        case ErrorLevel::SYNTAX:   return "Syntax Error";
        case ErrorLevel::SEMANTIC: return "Semantic Error";
        case ErrorLevel::CODEGEN:  return "Code Generation Error";
        case ErrorLevel::RUNTIME:  return "Runtime Error";
        default:                   return "Unknown Error";
    }
}

void ErrorHandler::error(ErrorLevel level, const Token& token, const std::string& message) {
    error(level, token.line, token.column, message);
}

void ErrorHandler::error(ErrorLevel level, int line, int column, const std::string& message) {
    errors.emplace_back(level, line, column, message);
    
    // Print error to stderr immediately with color coding
    std::cerr << "\033[1;31m" << getLevelString(level) << "\033[0m"  // Red color for error level
              << " at line " << line << ", column " << column 
              << ": " << message << std::endl;
}

void ErrorHandler::errorWithContext(ErrorLevel level, const Token& token, 
                                  const std::string& message, const std::string& sourceCode) {
    // Extract the relevant line from source code
    std::istringstream stream(sourceCode);
    std::string line;
    int currentLine = 1;
    
    while (std::getline(stream, line) && currentLine < token.line) {
        currentLine++;
    }
    
    std::string context = line + "\n" + 
                         std::string(token.column - 1, ' ') + "^";
    
    errors.emplace_back(level, token.line, token.column, message, context);
    
    // Print error with context
    std::cerr << "\033[1;31m" << getLevelString(level) << "\033[0m\n"
              << "At line " << token.line << ", column " << token.column << ":\n"
              << context << "\n"
              << message << std::endl;
}

bool ErrorHandler::hasErrors(ErrorLevel level) const {
    return std::any_of(errors.begin(), errors.end(),
                      [level](const Error& e) { return e.level == level; });
}

std::vector<ErrorHandler::Error> ErrorHandler::getErrors(ErrorLevel level) const {
    std::vector<Error> levelErrors;
    std::copy_if(errors.begin(), errors.end(), std::back_inserter(levelErrors),
                 [level](const Error& e) { return e.level == level; });
    return levelErrors;
}

size_t ErrorHandler::getErrorCount(ErrorLevel level) const {
    return std::count_if(errors.begin(), errors.end(),
                        [level](const Error& e) { return e.level == level; });
}

void ErrorHandler::clearErrors(ErrorLevel level) {
    errors.erase(
        std::remove_if(errors.begin(), errors.end(),
                      [level](const Error& e) { return e.level == level; }),
        errors.end());
}