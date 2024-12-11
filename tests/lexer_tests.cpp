#include <gtest/gtest.h>
#include "lexer.h"
#include "error_handler.h"

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing errors before each test
        ErrorHandler::instance().clearAllErrors();
    }
};

// Test invalid float literals
TEST_F(LexerTest, InvalidFloatLiterals) {
    std::string input = "var x: float = 3..; var y: float = 3.";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    // Check if errors were reported
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
    EXPECT_EQ(errors.size(), 2);
    
    // Check error messages
    EXPECT_TRUE(errors[0].message.find("multiple decimal points") != std::string::npos);
    EXPECT_TRUE(errors[1].message.find("needs at least one digit after decimal point") != std::string::npos);
}

// Test unterminated string literals
TEST_F(LexerTest, UnterminatedString) {
    std::string input = "var name: str = \"hello world";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
    EXPECT_EQ(errors.size(), 1);
    EXPECT_TRUE(errors[0].message.find("Unterminated string literal") != std::string::npos);
}

// Test invalid escape sequences
TEST_F(LexerTest, InvalidEscapeSequence) {
    std::string input = "var str: str = \"hello\\k world\"";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
    EXPECT_EQ(errors.size(), 1);
    EXPECT_TRUE(errors[0].message.find("Invalid escape sequence") != std::string::npos);
}

// Test invalid operators
TEST_F(LexerTest, InvalidOperators) {
    std::string input = "if (x & y) { } if (x | y) { }";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
    EXPECT_EQ(errors.size(), 2);
    EXPECT_TRUE(errors[0].message.find("Expected '&&'") != std::string::npos);
    EXPECT_TRUE(errors[1].message.find("Expected '||'") != std::string::npos);
}

// Test invalid characters
TEST_F(LexerTest, InvalidCharacters) {
    std::string input = "var x: int = 42; # comment";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
    EXPECT_EQ(errors.size(), 1);
    EXPECT_TRUE(errors[0].message.find("Unexpected character '#'") != std::string::npos);
}

// Test error recovery
TEST_F(LexerTest, ErrorRecovery) {
    std::string input = "var x: int = 3..; var y: int = 42;";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    // Check that we got errors but also valid tokens
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    
    // Find the valid integer token '42'
    bool found42 = false;
    for (const auto& token : tokens) {
        if (token.type == NUMBER && token.value == "42") {
            found42 = true;
            break;
        }
    }
    EXPECT_TRUE(found42);
}

// Test line and column tracking
TEST_F(LexerTest, LineColumnTracking) {
    std::string input = "var x: int = 42;\nvar str: str = \"unterminated";
    Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    EXPECT_TRUE(ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL));
    auto errors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
    EXPECT_EQ(errors.size(), 1);
    EXPECT_EQ(errors[0].line, 2);  // Error should be on line 2
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


