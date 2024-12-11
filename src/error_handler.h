#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include "lexer.h"
#include "token.h"


enum class ErrorLevel {
    LEXICAL,    // Token/character level errors
    SYNTAX,     // Grammar/parsing errors
    SEMANTIC,   // Type checking, scope errors
    CODEGEN,    // Code generation errors
    RUNTIME     // Runtime errors (for interpreter mode)
};

class ErrorHandler {
public:
    struct Error {
        ErrorLevel level;
        int line;
        int column;
        std::string message;
        std::string sourceSnippet;  // Context from source code
        
        Error(ErrorLevel lvl, int l, int c, const std::string& msg, const std::string& snippet = "") 
            : level(lvl), line(l), column(c), message(msg), sourceSnippet(snippet) {}
    };

    // Report an error at a specific token with error level
    void error(ErrorLevel level, const Token& token, const std::string& message);
    
    // Report an error at a specific location with error level
    void error(ErrorLevel level, int line, int column, const std::string& message);

    // Report an error with source code context
    void errorWithContext(ErrorLevel level, const Token& token, 
                         const std::string& message, const std::string& sourceCode);
    
    // Check if any errors have been reported for a specific level
    bool hasErrors(ErrorLevel level) const;
    
    // Check if any errors have been reported at all
    bool hasErrors() const { return !errors.empty(); }
    
    // Get errors for a specific level
    std::vector<Error> getErrors(ErrorLevel level) const;
    
    // Get all errors
    const std::vector<Error>& getAllErrors() const { return errors; }
    
    // Get error count for a specific level
    size_t getErrorCount(ErrorLevel level) const;
    
    // Get total error count
    size_t getTotalErrorCount() const { return errors.size(); }
    
    // Clear errors for a specific level
    void clearErrors(ErrorLevel level);
    
    // Clear all errors
    void clearAllErrors() { errors.clear(); }

    // Get string representation of error level
    static std::string getLevelString(ErrorLevel level);

    // Static method to get singleton instance
    static ErrorHandler& instance() {
        static ErrorHandler handler;
        return handler;
    }

private:
    std::vector<Error> errors;
    
    // Private constructor for singleton pattern
    ErrorHandler() = default;
    
    // Prevent copying
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
};

#endif