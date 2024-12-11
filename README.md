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
- Function overloading
- Multi-file preprocessing and compilation support
- Optimization passes
- Basic OOP support


## Lei Language Grammar

### Program Structure
```ebnf
program        → function*
function       → "fn" type IDENTIFIER "(" parameters? ")" block
parameters     → parameter ("," parameter)*
parameter      → IDENTIFIER ":" type

### Types
type           → basicType | arrayType
basicType      → "int" | "float" | "bool" | "str"
arrayType      → basicType "[" (NUMBER | "dynamic")? "]"  # Fixed size or dynamic arrays

### Statements
block          → "{" statement* "}"
statement      → varDecl 
               | ifStmt 
               | whileStmt 
               | returnStmt 
               | printStmt
               | exprStmt

varDecl        → "var" IDENTIFIER ":" type ("=" initializer)? ";"
initializer    → expression 
               | arrayInitializer
               | "new" basicType "[" expression "]"  # Dynamic array allocation

arrayInitializer → "{" (expression ("," expression)*)? "}"  # Can be empty
ifStmt         → "if" expression block ("else" block)?
whileStmt      → "while" expression block
returnStmt     → "return" expression? ";"
printStmt      → "print" "(" expression ")" ";"
exprStmt       → expression ";"

### Expressions
expression     → assignment
assignment     → (call ".")? IDENTIFIER assignOp expression
               | logicalOr
assignOp       → "=" | "+=" | "-=" | "*=" | "/="

logicalOr      → logicalAnd ("||" logicalAnd)*
logicalAnd     → equality ("&&" equality)*
equality       → comparison (("==" | "!=") comparison)*
comparison     → term (("<" | "<=" | ">" | ">=") term)*
term           → factor (("+" | "-") factor)*
factor         → unary (("*" | "/") unary)*
unary          → ("!" | "-") unary | call
call           → primary ("(" arguments? ")" | "[" expression "]")*
primary        → NUMBER | STRING | "true" | "false" | "(" expression ")"
               | IDENTIFIER | arrayInitializer
               | arrayAllocation

arrayAllocation → "new" basicType "[" expression "]"
arguments      → expression ("," expression)*

### Lexical Rules
NUMBER         → DIGIT+ ("." DIGIT+)?
STRING         → "\"" <any char except "\"">* "\""
IDENTIFIER     → ALPHA (ALPHA | DIGIT)*
ALPHA          → "a"..."z" | "A"..."Z" | "_"
DIGIT          → "0"..."9"
```

### Variable Initialization Rules

1. **Basic Types Default Values**
   ```lei
   var i: int;           // Defaults to 0
   var f: float;         // Defaults to 0.0
   var b: bool;          // Defaults to false
   var s: str;           // Defaults to ""
   ```

2. **Fixed-Size Arrays**
   ```lei
   var arr1: int[5];               // Array of 5 ints, initialized to [0,0,0,0,0]
   var arr2: float[3] = {1.0};     // Partially initialized to [1.0,0.0,0.0]
   var arr3: bool[2] = {};         // Empty initialization [false,false]
   ```

3. **Dynamic Arrays**
   ```lei
   var arr4: int[];                // Dynamic array, initially empty
   arr4 = new int[10];            // Allocate 10 elements
   
   var size: int = input();
   var arr5: float[] = new float[size];  // Runtime size determination
   ```

### Example Code
```lei
fn int main() {
    // Uninitialized variables with default values
    var count: int;            // 0
    var temperature: float;    // 0.0
    
    // Fixed size array
    var fixed: int[3];         // [0,0,0]
    
    // Dynamic array allocation
    var size: int = 5;
    var dynamic: int[] = new int[size];
    
    // Partial array initialization
    var grades: float[5] = {90.5, 85.0};  // [90.5, 85.0, 0.0, 0.0, 0.0]
    
    return 0;
}
```

### Notable Additions and Rules

1. **Default Initialization**
   - All basic types have default values when uninitialized
   - Array elements are initialized to their type's default value

2. **Dynamic Arrays**
   - Can be declared without size using `type[]`
   - Size determined at runtime using `new type[size]`
   - Support reallocation with new size

3. **Array Initialization**
   - Can be fully initialized, partially initialized, or empty
   - Missing elements get default values
   - Fixed-size arrays maintain their size constraint
   - Dynamic arrays can be resized

4. **Memory Management**
   - Dynamic arrays are automatically managed
   - Memory is freed when arrays go out of scope
   - No explicit delete/free operations needed

5. **Constraints**
   - Cannot access array elements before allocation
   - Array indices must be within bounds
   - Dynamic arrays must be allocated before use