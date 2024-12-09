# LEI: Implementing programming language with LLVM

## Getting started

If your station of building is Windows, I would strongly recommend to use linux core subsystem (WSL), for sure you could do the way you prefer. 



## Purpose

Educational!

I am interrested in language design and want to try develop something actually working for the fisrt time in the domain of concerns. I hope by the end it is at least ***Turing Complete*** and it is indeed!

## Project Related Assumptions

- *Target Architecture*: x8086
- *Binary File Format*: ELF64/PE. Depends on the build environment flags/switches.
- *Implementation language*: C++
- *Build generator and build system*: Cmake, Make

## What's the language is about...

### Grammar
```lisp
## Top-Level Structure
<program> ::= <main_function> {<function>}

<main_function> ::= "fn" ("int" | "void") "main" "(" [<parameters>] ")" "{" {<statement>} "}"

<function> ::= "fn" <type> <identifier> "(" [<parameters>] ")" "{" {<statement>} "}"

## Parameters and Types
<parameters> ::= <parameter> {"," <parameter>}
<parameter> ::= <type> <identifier>

<type> ::= "int" | "char" | "str" | "bool" | "void"

## Expressions with Explicit Precedence
<expression> ::= <logical_or_expression>

<logical_or_expression> ::= <logical_and_expression> 
                           {("||") <logical_and_expression>}

<logical_and_expression> ::= <equality_expression> 
                            {("&&") <equality_expression>}

<equality_expression> ::= <comparison_expression> 
                         {("==" | "!=") <comparison_expression>}

<comparison_expression> ::= <additive_expression> 
                           {("<" | ">" | "<=" | ">=") <additive_expression>}

<additive_expression> ::= <multiplicative_expression> 
                         {("+" | "-") <multiplicative_expression>}

<multiplicative_expression> ::= <unary_expression> 
                               {("*" | "/") <unary_expression>}

<unary_expression> ::= <primary_expression>
                     | <function_call>
                     | <string_interpolation>

<primary_expression> ::= <literal>
                       | <identifier>
                       | "(" <expression> ")"

## Statements
<statement> ::= <variable_declaration> ";"
              | <assignment> ";"
              | <function_call> ";"
              | <return_statement> ";"
              | <if_statement>
              | <while_loop>
              | <expression> ";"

## Detailed Statement Definitions
<variable_declaration> ::= <type> <identifier> "=" <expression>
<assignment> ::= <identifier> "=" <expression>
<function_call> ::= <identifier> "(" [<arguments>] ")"
<return_statement> ::= "return" [<expression>]

<if_statement> ::= "if" <expression> "{" {<statement>} "}" 
                   ["else" "{" {<statement>} "}"]

<while_loop> ::= "while" <expression> "{" {<statement>} "}"

## Supporting Definitions
<arguments> ::= <expression> {"," <expression>}

<string_interpolation> ::= "\"" <string_part> "${" <identifier> "}" <string_part> "\""

## Literals and Identifiers
<literal> ::= <int_literal> | <char_literal> | <string_literal> | <bool_literal>
<int_literal> ::= [0-9]+
<char_literal> ::= "'" <character> "'"
<string_literal> ::= "\"" {<character>} "\""
<bool_literal> ::= "0" | "1"

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<string_part> ::= {<character>}
<character> ::= any printable ASCII character except '"' or '`'

```

## **Future Enhancements**
- **Optimizations**: To use LLVM’s optimization passes to improve the generated machine code.
- **Dynamic Linking**: To allow calling external functions (like `printf`) from your code.
- **REPL**: To Implement a Read-Eval-Print Loop for interactive execution.


## References

- https://github.com/ghaiklor/llvm-kaleidoscope/
- https://eli.thegreenplace.net/2013/02/25/a-deeper-look-into-the-llvm-code-generator-part-1

- https://classes.cs.uchicago.edu/archive/2020/fall/22600-1/project/llvm.pdf