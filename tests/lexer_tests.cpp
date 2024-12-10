/**
 * @file lexer_test.cpp
 * @brief Unit tests for the Lexer class using Google Test
 */

#include <gtest/gtest.h>
#include "lexer.h"

class LexerTest : public ::testing::Test {
protected:
    // Helper function to verify token sequence
    void verifyTokenSequence(const std::string& input, 
                           const std::vector<std::pair<TokenType, std::string>>& expected) {
        Lexer lexer(input);
        std::vector<Token> tokens = lexer.tokenize();
        
        ASSERT_EQ(tokens.size(), expected.size() + 1); // +1 for END token
        
        for (size_t i = 0; i < expected.size(); ++i) {
            EXPECT_EQ(tokens[i].type, expected[i].first)
                << "Token mismatch at position " << i;
            EXPECT_EQ(tokens[i].value, expected[i].second)
                << "Value mismatch at position " << i;
        }
        
        // Verify END token
        EXPECT_EQ(tokens.back().type, END);
    }
};

// Test function declarations
TEST_F(LexerTest, FunctionDeclaration) {
    const std::string input = "fn int add(a: int, b: int) { return a + b; }";
    const std::vector<std::pair<TokenType, std::string>> expected = {
        {FN, "fn"},
        {INT, "int"},
        {IDENTIFIER, "add"},
        {LPAREN, "("},
        {IDENTIFIER, "a"},
        {COLON, ":"},
        {INT, "int"},
        {COMMA, ","},
        {IDENTIFIER, "b"},
        {COLON, ":"},
        {INT, "int"},
        {RPAREN, ")"},
        {LBRACE, "{"},
        {RETURN, "return"},
        {IDENTIFIER, "a"},
        {PLUS, "+"},
        {IDENTIFIER, "b"},
        {SEMICOLON, ";"},
        {RBRACE, "}"}
    };
    
    verifyTokenSequence(input, expected);
}

// Test variable declarations
TEST_F(LexerTest, VariableDeclarations) {
    const std::string input = 
        "var x: float = 3.14;\n"
        "var b: bool = true;\n"
        "var i: int = 42;";
        
    const std::vector<std::pair<TokenType, std::string>> expected = {
        {VAR, "var"},
        {IDENTIFIER, "x"},
        {COLON, ":"},
        {FLOAT_TYPE, "float"},
        {EQUALS, "="},
        {FLOAT_LITERAL, "3.14"},
        {SEMICOLON, ";"},
        {VAR, "var"},
        {IDENTIFIER, "b"},
        {COLON, ":"},
        {BOOL_TYPE, "bool"},
        {EQUALS, "="},
        {BOOL_LITERAL, "true"},
        {SEMICOLON, ";"},
        {VAR, "var"},
        {IDENTIFIER, "i"},
        {COLON, ":"},
        {INT, "int"},
        {EQUALS, "="},
        {NUMBER, "42"},
        {SEMICOLON, ";"}
    };
    
    verifyTokenSequence(input, expected);
}

// Test array operations
TEST_F(LexerTest, ArrayOperations) {
    const std::string input = "var arr: float[5] = {1.12, 2.143, 3.12};";
    
    const std::vector<std::pair<TokenType, std::string>> expected = {
        {VAR, "var"},
        {IDENTIFIER, "arr"},
        {COLON, ":"},
        {FLOAT_TYPE, "float"},
        {LBRACKET, "["},
        {NUMBER, "5"},
        {RBRACKET, "]"},
        {EQUALS, "="},
        {LBRACE, "{"},
        {FLOAT_LITERAL, "1.12"},
        {COMMA, ","},
        {FLOAT_LITERAL, "2.143"},
        {COMMA, ","},
        {FLOAT_LITERAL, "3.12"},
        {RBRACE, "}"},
        {SEMICOLON, ";"}
    };
    
    verifyTokenSequence(input, expected);
}

// Test error handling
TEST_F(LexerTest, ErrorHandling) {
    // Test invalid number
    EXPECT_THROW({
        Lexer lexer("var x: float = 3.14.15;");
        lexer.tokenize();
    }, LexerError);
    
    // Test unterminated string
    EXPECT_THROW({
        Lexer lexer("var str: str = \"unterminated;");
        lexer.tokenize();
    }, LexerError);
    
    // Test invalid character
    EXPECT_THROW({
        Lexer lexer("var x: int = @;");
        lexer.tokenize();
    }, LexerError);
}

// Test comments
TEST_F(LexerTest, Comments) {
    const std::string input = 
        "// This is a comment\n"
        "var x: int = 42; // Inline comment\n"
        "// Another comment";
        
    const std::vector<std::pair<TokenType, std::string>> expected = {
        {VAR, "var"},
        {IDENTIFIER, "x"},
        {COLON, ":"},
        {INT, "int"},
        {EQUALS, "="},
        {NUMBER, "42"},
        {SEMICOLON, ";"}
    };
    
    verifyTokenSequence(input, expected);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}