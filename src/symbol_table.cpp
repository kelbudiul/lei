#include "symbol_table.h"

void SymbolTable::add(const std::string &name, const std::string &type) {
    if (table.find(name) != table.end()) {
        throw std::runtime_error("Semantic Error: Duplicate declaration of " + name);
    }
    table[name] = {name, type};
}

Symbol SymbolTable::get(const std::string &name) {
    if (table.find(name) == table.end()) {
        throw std::runtime_error("Semantic Error: Undefined identifier " + name);
    }
    return table[name];
}

bool SymbolTable::exists(const std::string &name) const {
    return table.find(name) != table.end();
}

void SymbolTable::clear() {
    table.clear();
}