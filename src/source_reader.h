#pragma once

#include <string>

namespace Lei
{
    class SourceReader {
        public:
        // Read entire file into a string
        static std::string readSourceFile(const std::string& filename);

        // Read file line by line
        static bool readSourceFileLines(const std::string& filename);
};
} // namespace Lei

