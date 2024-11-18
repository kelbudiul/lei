#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>


class LeiCompiler
{
private:
    std::string sourceFilePath;
    std::vector<std::string> sourceLines;

public:
    LeiCompiler(){};
    // Constructor to initialize with source file path
    LeiCompiler(const std::string& filePath) : sourceFilePath(filePath) {}

    // Read source code from file
    bool readSourceFile() {
        std::ifstream sourceFile(sourceFilePath);
        if (!sourceFile.is_open()) {
            std::cerr << "Error: Could not open file " << sourceFilePath << std::endl;
            return false;
        }

        // Clear previous lines
        sourceLines.clear();
        
        // Read file line by line
        std::string line;
        while (std::getline(sourceFile, line)) {
            // Optional: Trim whitespace and skip empty lines
            if (!line.empty() && line.find_first_not_of(" \t") != std::string::npos) {
                sourceLines.push_back(line);
            }
        }

        sourceFile.close();
        return true;
    }

    // Print source lines for verification
    void printSourceLines() {
        std::cout << "Source Code:" << std::endl;
        for (size_t i = 0; i < sourceLines.size(); ++i) {
            std::cout << i + 1 << ": " << sourceLines[i] << std::endl;
        }
    }

    // Getter for source lines
    const std::vector<std::string>& getSourceLines() const {
        return sourceLines;
    }
};


int main(int argc, char const *argv[])
{
    LeiCompiler compiler;

    
    return 0;
}
