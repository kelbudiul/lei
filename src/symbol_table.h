#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "../include/common.h"

// Symbol representation
struct Symbol {
    std::string name;
    std::string type;
};

// Symbol Table management
class SymbolTable {
private:
    std::unordered_map<std::string, Symbol> table;

public:
    void add(const std::string &name, const std::string &type);
    Symbol get(const std::string &name);
    bool exists(const std::string &name) const;
    void clear();
};

#endif // SYMBOL_TABLE_H