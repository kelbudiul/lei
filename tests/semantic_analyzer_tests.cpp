#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "semantic_visitor.h"
#include "error_handler.h"

class SemanticAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ErrorHandler::instance().clearAllErrors();
    }

    std::unique_ptr<Program> parse(const std::string& source) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL)) return nullptr;
        
        Parser parser(tokens);
        return parser.parse();
    }

    bool analyze(const std::string& source) {
        auto ast = parse(source);
        if (!ast) return false;

        SemanticAnalyzer analyzer;
        return analyzer.analyze(ast.get());
    }

    bool hasSemanticError(const std::string& source, const std::string& expectedError) {
        analyze(source);
        auto errors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
        
        for (const auto& error : errors) {
            if (error.message.find(expectedError) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

// Test main function validation
TEST_F(SemanticAnalyzerTest, MainFunctionValidation) {
    // Test valid main function signatures
    EXPECT_TRUE(analyze("fn int main() { return 0; }"));
    EXPECT_TRUE(analyze("fn int main(argc: int, argv: str[]) { return 0; }"));

    // Test invalid return type
    std::string invalidReturnType = R"(
        fn void main() { 
            return;
        }
    )";
    EXPECT_TRUE(analyze(invalidReturnType));  // Should still parse successfully
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
    bool foundReturnTypeError = false;
    for (const auto& error : errors) {
        if (error.message.find("Main function must return int") != std::string::npos) {
            foundReturnTypeError = true;
            break;
        }
    }
    EXPECT_TRUE(foundReturnTypeError);
}

// Test variable declarations and type checking
TEST_F(SemanticAnalyzerTest, VariableDeclarations) {
    // Valid declarations with initialization
    EXPECT_TRUE(analyze(R"(
        fn int main() {
            var x: int = 42;
            var y: float = 3.14;
            var s: str = "hello";
            var b: bool = true;
            return 0;
        }
    )"));

    // Type mismatch in initialization
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int main() {
            var x: int = "string";
            return 0;
        }
        )",
        "Type mismatch in variable declaration"
    ));

    // Duplicate variable declaration
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int main() {
            var x: int = 1;
            var x: int = 2;
            return 0;
        }
        )",
        "Variable already declared in this scope"
    ));
}

// Test array operations
TEST_F(SemanticAnalyzerTest, ArrayOperations) {
    // Test invalid array index type
    std::string invalidIndex = R"(
        fn int main() {
            var arr: int[] = {1, 2, 3};
            arr[true] = 42;  // Should error: boolean index
            return 0;
        }
    )";
    EXPECT_TRUE(analyze(invalidIndex));  // Should still parse
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
    bool foundIndexError = false;
    for (const auto& error : errors) {
        if (error.message.find("Array index must be an integer") != std::string::npos) {
            foundIndexError = true;
            break;
        }
    }
    EXPECT_TRUE(foundIndexError);

    // Clear errors for next test
    ErrorHandler::instance().clearAllErrors();

    // Test incompatible array element types
    std::string incompatibleTypes = R"(
        fn int main() {
            var arr: int[] = {1, "string", 3};  // Should error: mixed types
            return 0;
        }
    )";
    EXPECT_TRUE(analyze(incompatibleTypes));
    errors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
    bool foundTypeError = false;
    for (const auto& error : errors) {
        if (error.message.find("Array elements must have compatible types") != std::string::npos) {
            foundTypeError = true;
            break;
        }
    }
    EXPECT_TRUE(foundTypeError);
}

// Test function declarations and calls
TEST_F(SemanticAnalyzerTest, FunctionCallsAndReturns) {
    // Valid function declaration and call
    EXPECT_TRUE(analyze(R"(
        fn int add(a: int, b: int) {
            return a + b;
        }

        fn int main() {
            var result: int = add(1, 2);
            return 0;
        }
    )"));

    // Wrong argument count
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int add(a: int, b: int) {
            return a + b;
        }

        fn int main() {
            var result: int = add(1);
            return 0;
        }
        )",
        "Wrong number of arguments"
    ));

    // Argument type mismatch
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int add(a: int, b: int) {
            return a + b;
        }

        fn int main() {
            var result: int = add("one", "two");
            return 0;
        }
        )",
        "Argument type mismatch"
    ));

    // Return type mismatch
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int getValue() {
            return "string";
        }

        fn int main() {
            return 0;
        }
        )",
        "Return type mismatch"
    ));
}

// Test control flow conditions
TEST_F(SemanticAnalyzerTest, ControlFlowConditions) {
    // Valid conditions
    EXPECT_TRUE(analyze(R"(
        fn int main() {
            var x: int = 0;
            var b: bool = true;

            if b {
                x = 1;
            }

            while b {
                x = x + 1;
                if x > 10 {
                    b = false;
                }
            }
            return 0;
        }
    )"));

    // Non-boolean if condition
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int main() {
            if 42 {
                return 1;
            }
            return 0;
        }
        )",
        "If condition must evaluate to a boolean value"
    ));

    // Non-boolean while condition
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int main() {
            while "forever" {
                return 1;
            }
            return 0;
        }
        )",
        "While condition must evaluate to a boolean value"
    ));
}

// Test operators and expressions
TEST_F(SemanticAnalyzerTest, OperatorsAndExpressions) {
    // Test invalid arithmetic operands
    std::string invalidArithmetic = R"(
        fn int main() {
            var x: int = true + 42;  // Should error: can't add bool and int
            return 0;
        }
    )";
    EXPECT_TRUE(analyze(invalidArithmetic));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
    bool foundArithmeticError = false;
    for (const auto& error : errors) {
        if (error.message.find("Invalid operand types") != std::string::npos ||
            error.message.find("Type mismatch") != std::string::npos) {
            foundArithmeticError = true;
            break;
        }
    }
    EXPECT_TRUE(foundArithmeticError);

    // Clear errors for next test
    ErrorHandler::instance().clearAllErrors();

    // Test invalid logical operands
    std::string invalidLogical = R"(
        fn int main() {
            var b: bool = 1 && 2;  // Should error: && requires boolean operands
            return 0;
        }
    )";
    EXPECT_TRUE(analyze(invalidLogical));
    errors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
    bool foundLogicalError = false;
    for (const auto& error : errors) {
        if (error.message.find("Invalid operand types") != std::string::npos ||
            error.message.find("requires boolean operand") != std::string::npos) {
            foundLogicalError = true;
            break;
        }
    }
    EXPECT_TRUE(foundLogicalError);
}

// Test scope rules
TEST_F(SemanticAnalyzerTest, ScopeRules) {
    // Valid scoping
    EXPECT_TRUE(analyze(R"(
        fn int main() {
            var x: int = 1;
            {
                var y: int = 2;
                x = y;  // OK: y is in scope
            }
            return 0;
        }
    )"));

    // Variable not in scope
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn int main() {
            {
                var x: int = 1;
            }
            x = 2;  // Error: x not in scope
            return 0;
        }
        )",
        "Undefined variable"
    ));

    // Function scope isolation
    EXPECT_TRUE(hasSemanticError(
        R"(
        fn void foo() {
            var x: int = 1;
        }

        fn int main() {
            x = 2;  // Error: x not in scope
            return 0;
        }
        )",
        "Undefined variable"
    ));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}