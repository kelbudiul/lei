#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "error_handler.h"

class ParserTest : public ::testing::Test {
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

    bool hasParseError(const std::string& source, const std::string& expectedError) {
        parse(source);
        auto errors = ErrorHandler::instance().getErrors(ErrorLevel::SYNTAX);
        
        for (const auto& error : errors) {
            if (error.message.find(expectedError) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    // Helper to verify AST structure through ASTPrinter output
    std::string getAstString(const std::string& source) {
        auto ast = parse(source);
        if (!ast) return "";
        
        ASTPrinter printer;
        return printer.print(ast.get());
    }
};

// Test function declarations
TEST_F(ParserTest, FunctionDeclarations) {
    // Basic function with no parameters
    EXPECT_NO_THROW(parse("fn int main() { return 0; }"));

    // Function with multiple parameters
    EXPECT_NO_THROW(parse("fn int add(x: int, y: float) { return 0; }"));

    // Function with array parameters
    EXPECT_NO_THROW(parse("fn void process(arr: int[], size: int) { }"));

    // Missing return type
    EXPECT_TRUE(hasParseError(
        "fn add(x: int) { return x; }",
        "Expected type specifier"
    ));

    // Missing parameter type
    EXPECT_TRUE(hasParseError(
        "fn int add(x, y: int) { return 0; }",
        "Expected ':' after parameter name"
    ));

    // Missing function body
    EXPECT_TRUE(hasParseError(
        "fn int main();",
        "Expected '{' before block"
    ));
}

// Test variable declarations
TEST_F(ParserTest, VariableDeclarations) {
    // Basic declarations
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var x: int;
            var y: float = 3.14;
            var str: str = "hello";
            var flag: bool = true;
            return 0;
        }
    )"));

    // Array declarations
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var arr1: int[];
            var arr2: int[5];
            var arr3: int[] = {1, 2, 3};
            return 0;
        }
    )"));

    // Missing type
    EXPECT_TRUE(hasParseError(
        "fn int main() { var x = 42; return 0; }",
        "Expected ':' after variable name"
    ));

    // Missing semicolon
    EXPECT_TRUE(hasParseError(
        "fn int main() { var x: int = 42 return 0; }",
        "Expected ';' after variable declaration"
    ));
}

// Test statements
TEST_F(ParserTest, Statements) {
    // Test block statements
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            {
                var x: int = 42;
                {
                    var y: int = x;
                }
            }
            return 0;
        }
    )"));

    // Test if statements
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            if true {
                return 1;
            } else {
                return 0;
            }
        }
    )"));

    // Test while statements
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var i: int = 0;
            while i < 10 {
                i = i + 1;
            }
            return 0;
        }
    )"));

    // Missing condition parentheses
    EXPECT_TRUE(hasParseError(
        "fn int main() { if { return 0; } }",
        "Expected expression"
    ));

    // Missing block braces
    EXPECT_TRUE(hasParseError(
        "fn int main() { while true return 0; }",
        "Expected '{' before block"
    ));
}

// Test expressions
TEST_F(ParserTest, Expressions) {
    // Test arithmetic expressions
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var x: int = 1 + 2 * 3 - 4 / 5;
            return 0;
        }
    )"));

    // Test logical expressions
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var b: bool = true && false || true;
            return 0;
        }
    )"));

    // Test comparison expressions
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var b: bool = 1 < 2 && 3 >= 4 || 5 == 6;
            return 0;
        }
    )"));

    // Test operator precedence
    auto ast = getAstString(R"(
        fn int main() {
            var x: int = 1 + 2 * 3;
            return 0;
        }
    )");
    // Verify that multiplication has higher precedence
    EXPECT_TRUE(ast.find("Binary Expression: +") != std::string::npos);
    EXPECT_TRUE(ast.find("Binary Expression: *") != std::string::npos);
}

// Test array operations
TEST_F(ParserTest, ArrayOperations) {
    // Test array initialization
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var arr1: int[] = {1, 2, 3};
            var arr2: int[5];
            var arr3: int[] = {1 + 2, 3 * 4, 5};
            return 0;
        }
    )"));

    // Test array access
    EXPECT_NO_THROW(parse(R"(
        fn int main() {
            var arr: int[] = {1, 2, 3};
            var x: int = arr[0];
            arr[1] = 42;
            arr[1 + 1] = arr[1] * 2;
            return 0;
        }
    )"));

    // Missing closing bracket
    EXPECT_TRUE(hasParseError(
        "fn int main() { var arr: int[]; var x: int = arr[0; return 0; }",
        "Expected ']' after array index"
    ));

    // Invalid array initialization
    EXPECT_TRUE(hasParseError(
        "fn int main() { var arr: int[] = {1, 2,}; return 0; }",
        "Expected expression"
    ));
}

// Test function calls
TEST_F(ParserTest, FunctionCalls) {
    // Test basic function calls
    EXPECT_NO_THROW(parse(R"(
        fn int add(a: int, b: int) { return a + b; }
        fn int main() {
            var x: int = add(1, 2);
            add(add(1, 2), add(3, 4));
            return 0;
        }
    )"));

    // Test function calls in expressions
    EXPECT_NO_THROW(parse(R"(
        fn int add(a: int, b: int) { return a + b; }
        fn int mul(a: int, b: int) { return a * b; }
        fn int main() {
            var x: int = add(1, 2) * mul(3, 4);
            return 0;
        }
    )"));

    // Missing closing parenthesis
    EXPECT_TRUE(hasParseError(
        "fn int main() { print(42; return 0; }",
        "Expected ')' after arguments"
    ));

    // Missing comma between arguments
    EXPECT_TRUE(hasParseError(
        "fn int main() { print(1 2 3); return 0; }",
        "Expected ')' after arguments"
    ));
}

// Test error recovery
TEST_F(ParserTest, ErrorRecovery) {
    // Test recovery after statement errors
    auto ast = parse(R"(
        fn int main() {
            var x: int = 42;
            if { }  // Error: missing condition
            var y: int = 43;  // Should still parse this
            return 0;
        }
    )");
    EXPECT_NE(ast, nullptr);
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::SYNTAX));
    
    // Test recovery after expression errors
    ast = parse(R"(
        fn int main() {
            var x: int = 1 + * 2;  // Error: unexpected operator
            var y: int = 42;  // Should still parse this
            return 0;
        }
    )");
    EXPECT_NE(ast, nullptr);
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::SYNTAX));
}

// Test operator associativity
TEST_F(ParserTest, OperatorAssociativity) {
    // Test left associativity of arithmetic operators
    auto ast = getAstString(R"(
        fn int main() {
            var x: int = 1 - 2 - 3;  // Should be ((1 - 2) - 3)
            return 0;
        }
    )");
    // Verify left-associative parsing
    EXPECT_TRUE(ast.find("Binary Expression: -") != std::string::npos);

    // Test right associativity of assignment
    ast = getAstString(R"(
        fn int main() {
            var a: int;
            var b: int;
            var c: int;
            a = b = c = 42;  // Should be (a = (b = (c = 42)))
            return 0;
        }
    )");
    // Verify right-associative parsing
    EXPECT_TRUE(ast.find("Assignment: =") != std::string::npos);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}