# Lei Programming Language

A statically-typed programming language compiler that generates LLVM IR.

## Features
- Static typing with support for `int`, `float`, `bool`, `str`, and `void` types
- Fixed-size and dynamic arrays
- Functions with proper scoping
- Memory management (malloc/realloc/free)
- Control flow statements (if/else, while)
- Standard I/O operations
- Compound assignment operators

## Development Environment Setup

### Prerequisites
- C++ compiler supporting C++17 or later
- LLVM development libraries (version 15.0 or later)
- CMake (version 3.13 or later)
- Git for version control

### Installation

#### Ubuntu/Debian
```bash
# Install build essentials and LLVM
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install llvm-dev
sudo apt-get install cmake
sudo apt-get install git

# Clone the repository
git clone https://github.com/yourusername/lei.git
cd lei

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run tests
make test
```

#### macOS
```bash
# Install prerequisites using Homebrew
brew install llvm
brew install cmake

# Clone and build (same steps as above)
```

#### Windows
1. Install Visual Studio with C++ support
2. Install LLVM from https://releases.llvm.org/
3. Install CMake from https://cmake.org/download/
4. Follow the build steps above using Visual Studio Developer Command Prompt

## Usage

### Command Line Options
```bash
leic [input] [-o output] [-e] [--print-ast] [--print-sp] [--print-ir]

Options:
    input           Input source file
    -o, --output    Output path for generated LLVM IR
    -e, --execute   Directly execute the generated LLVM IR
    --print-ast     Print the abstract syntax tree
    --print-sp      Print the symbol table
    --print-ir      Print the LLVM IR
```

### Example
```bash
# Compile a source file to LLVM IR
leic example.lei -o example.ll

# Compile and execute
leic example.lei -e

# Compile with debug output
leic example.lei --print-ast --print-ir
```

## Language Syntax Examples

### Basic Program Structure
```rust
// Function declaration with return type
fn int main() {
    var x: int = 42;        // Variable declaration with type annotation
    print(x);               // Built-in print function
    return 0;
}
```

### Variable Declarations and Types
```rust
fn void examples() {
    // Basic types
    var count: int = 0;            // Integer
    var temp: float = 98.6;        // Floating point
    var name: str = "Alice";       // String
    var isActive: bool = true;     // Boolean

    // Arrays
    var fixed: int[3] = {1, 2, 3}; // Fixed-size array
    var size: int = 5;
    var dynamic: int[] = malloc(size * sizeof(int)); // Dynamic array

    // Don't forget to free dynamic memory
    free(dynamic);
}
```

### Control Flow
```rust
fn int checkValue(x: int) {
    // If-else statement
    if x > 10 {
        print("Greater than 10");
    } else if x < 0 {
        print("Negative number");
    } else {
        print("Between 0 and 10");
    }

    // While loop
    var i: int = 0;
    while i < x {
        print(i);
        i = i + 1;
    }

    return x;
}
```

### Array Operations
```rust
// Function to sum array elements
fn int sum(arr: int[], size: int) {
    var total: int = 0;
    var i: int = 0;
    
    while i < size {
        total = total + arr[i];
        i = i + 1;
    }
    
    return total;
}

fn void arrayExample() {
    // Fixed array initialization
    var nums: int[5] = {1, 2, 3, 4, 5};
    
    // Dynamic array
    var dynamic: int[] = malloc(3 * sizeof(int));
    dynamic[0] = 10;
    dynamic[1] = 20;
    dynamic[2] = 30;
    
    // Resize dynamic array
    dynamic = realloc(dynamic, 4 * sizeof(int));
    dynamic[3] = 40;
    
    // Don't forget to free
    free(dynamic);
}
```

### String Operations
```rust
fn void stringExample() {
    var name: str = "John";
    var greeting: str = "Hello, ";
    
    // String input
    var input: str = input("Enter your name: ");
    
    // Print with string concatenation
    print(greeting + input);
}
```

### Compound Assignment Operators
```rust
fn void operatorExample() {
    var x: int = 5;
    x += 3;      // x = x + 3
    x -= 2;      // x = x - 2
    x *= 4;      // x = x * 4
    x /= 2;      // x = x / 2
}
```

### Memory Management Example
```rust
fn int[] createAndFillArray(size: int, value: int) {
    // Allocate memory
    var arr: int[] = malloc(size * sizeof(int));
    
    // Fill array
    var i: int = 0;
    while i < size {
        arr[i] = value;
        i = i + 1;
    }
    
    return arr;  // Caller is responsible for freeing memory
}

fn void memoryExample() {
    var arr: int[] = createAndFillArray(5, 42);
    
    // Use array
    print(arr[0]);
    
    // Clean up
    free(arr);
}
```

## Current Limitations

- Basic type system
- Single-file compilation only
- Basic error recovery

## Future Enhancements

Planned features include:
- Enhanced type system
- Function overloading
- Multi-file preprocessing and compilation support
- Optimization passes
- Basic OOP support


## Language Grammar

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
