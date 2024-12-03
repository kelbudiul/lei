#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "source_reader.h"


/**
 * The `SourceReader` helper class provides functions to read the entire content of a source file into
 * a string and to read the file line by line for further processing.
 * 
 * @param filename The functions `readSourceFile` and `readSourceFileLines` are designed to read the
 * contents of a file specified by the `filename` parameter.
 * 
 * @return In the `readSourceFile` function, a `std::string` containing the entire content of the file
 * is being returned. If the file cannot be opened, an empty string is returned.
 */

namespace Lei
{
    
    std::string SourceReader::readSourceFile(const std::string& filename) {
            std::ifstream file(filename);
            
            // Check if file opened successfully
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file " << filename << std::endl;
                return "";
            }

            // Read entire file into a string
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

    bool SourceReader::readSourceFileLines(const std::string& filename) {
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Process each line here
            // For example, you might want to pass it to a lexer or parser
            std::cout << "Read line: " << line << std::endl;
        }

        return true;
    }

} // namespace Lei
