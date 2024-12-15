#include <gtest/gtest.h>
#include "lexer.h"
#include "error_handler.h"
#include "parser.h"

class ParserTest(): public ::testing::Test {
    protected:
    void SetUp() override {
        // Clear any existing errors before each test
        ErrorHandler::instance().clearAllErrors();
    }
};


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}