# Lei Programming Language Compiler

A compiler implementation for the Lei programming language, built using C++ and LLVM. This project implements a complete compilation pipeline including lexical analysis, parsing, semantic analysis, and code generation.

## Project Structure

The compiler is organized into several key components:

### Core Components

- **Lexer** (`lexer.h`, `lexer.cpp`): Performs lexical analysis, converting source code into tokens. Handles basic language tokens including keywords (`fn`, `int`, `return`), identifiers, numbers, and symbols.

- **Parser** (`parser.h`, `parser.cpp`): Constructs an Abstract Syntax Tree (AST) from the token stream. Currently supports parsing of:
  - Function definitions
  - Return statements
  - Basic expressions

- **AST** (`ast.h`, `ast.cpp`): Defines the Abstract Syntax Tree structure using a visitor pattern. Includes nodes for:
  - Functions
  - Return statements
  - Declarations
  - Assignments
  - Arrays

- **Symbol Table** (`symbol_table.h`, `symbol_table.cpp`): Manages symbol information and scope, providing:
  - Symbol tracking
  - Type information storage
  - Duplicate declaration checking
  - Symbol lookup

### Analysis and Generation

- **Semantic Visitor** (`semantic_visitor.h`, `semantic_visitor.cpp`): Performs semantic analysis including:
  - Type checking
  - Symbol resolution
  - Error detection and reporting

- **Code Generator** (`codegen_visitor.h`, `codegen_visitor.cpp`): Generates LLVM IR and handles JIT compilation:
  - LLVM IR generation
  - Runtime execution support
  - Function compilation and execution

### Utility Components

- **Source Reader** (`source_reader.h`, `source_reader.cpp`): Handles source file operations:
  - File reading
  - Content streaming
  - Error handling for file operations

## Building and Running

### Prerequisites

- C++ compiler with C++17 support
- LLVM development libraries
- CMake (build system)

### Usage

```bash
./compiler <input_file> [output_path]
```

The compiler accepts two command-line arguments:
- `input_file`: Path to the source file (required)
- `output_path`: Output directory for generated files (optional, defaults to current directory)

## Implementation Details

### Visitor Pattern

The project extensively uses the visitor pattern for AST traversal and processing. Three main visitors are implemented:

1. Base Visitor (visitor.h)
2. Semantic Analysis Visitor
3. Code Generation Visitor

### Error Handling

The compiler implements comprehensive error handling:
- Lexical errors (unknown characters, malformed tokens)
- Parsing errors (unexpected tokens, syntax errors)
- Semantic errors (duplicate declarations, type mismatches)
- File handling errors

### Memory Management

The project uses modern C++ memory management practices:
- `std::unique_ptr` for AST node ownership
- RAII principles throughout
- Smart pointer usage for LLVM resources

## Current Limitations

- Basic type system (currently supports integers)
- Limited function support (no parameters yet)
- Single-file compilation only
- Basic error recovery

## Future Enhancements

Planned features include:
- Enhanced type system
- Function parameters and overloading
- Control flow statements
- Standard library implementation
- Multi-file compilation support
- Optimization passes


## Language Grammar Hierarchy

### 1. Program Structure
```ebnf
program        ::= declaration*
declaration    ::= functionDecl | varDecl | statement
```

### 2. Declarations
```ebnf
functionDecl   ::= "fn" type IDENTIFIER "(" paramList? ")" block
paramList      ::= param ("," param)*
param          ::= IDENTIFIER ":" type

varDecl        ::= "var" IDENTIFIER ":" type ("=" expression)?
```

### 3. Types
```ebnf
type          ::= "int" | "float" | "bool" | "str" | "void" | arrayType
arrayType     ::= type "[" "]"
```

### 4. Statements
```ebnf
statement     ::= exprStmt
                | block 
                | ifStmt
                | whileStmt
                | forStmt
                | returnStmt
                | varDecl

block         ::= "{" statement* "}"
ifStmt        ::= "if" expression block ("else" block)?
whileStmt     ::= "while" expression block
forStmt       ::= "for" IDENTIFIER "in" expression block
returnStmt    ::= "return" expression? ";"
```

### 5. Expressions (in order of precedence, lowest to highest)
```ebnf
expression    ::= assignment

assignment    ::= IDENTIFIER ("=" | "+=" | "-=" | "*=" | "/=") expression
                | logicOr

logicOr      ::= logicAnd ("or" logicAnd)*
logicAnd     ::= equality ("and" equality)*
equality     ::= comparison (("!=" | "==") comparison)*
comparison   ::= term ((">" | ">=" | "<" | "<=") term)*
term         ::= factor (("+" | "-") factor)*
factor       ::= unary (("*" | "/") unary)*
unary        ::= ("!" | "-") unary | primary

primary      ::= NUMBER | STRING | "true" | "false" | "(" expression ")"
               | IDENTIFIER | call | arrayAccess

call         ::= IDENTIFIER "(" arguments? ")"
arguments    ::= expression ("," expression)*
arrayAccess  ::= IDENTIFIER "[" expression "]"
```

### AST Node Hierarchy

```
ASTNode (base)
├── ExpressionNode
│   ├── LiteralExpr (int, float, bool, string)
│   ├── UnaryExpr (-, !)
│   ├── BinaryExpr (+, -, *, /, etc)
│   ├── ComparisonExpr (<, <=, >, >=, ==, !=)
│   ├── LogicalExpr (and, or)
│   ├── VariableExpr (identifier reference)
│   ├── AssignmentExpr (=, +=, -=, *=, /=)
│   ├── CallExpr (function calls)
│   └── ArrayAccessExpr
│
├── StatementNode
│   ├── ExpressionStmt
│   ├── BlockStmt
│   ├── IfStmt
│   ├── WhileStmt
│   ├── ForStmt
│   ├── ReturnStmt
│   └── VarDeclStmt
│
└── DeclarationNode
    ├── FunctionDecl
    └── VarDecl
```