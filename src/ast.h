#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>
#include <string>
#include "token.h"
#include "visitor.h"

// Forward declarations
class Visitor;

// Base AST node class
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(Visitor* visitor) = 0;
};

// Expression node base class
class Expr : public ASTNode {
public:
    virtual ~Expr() = default;
};

// Statement node base class
class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;
};

// Type representation
struct Type {
    std::string name;
    bool isArray;
    int arraySize;  // -1 for dynamic arrays
    
    Type(const std::string& n, bool arr = false, int size = 0)
        : name(n), isArray(arr), arraySize(size) {}
};

// Literal expression nodes
class NumberExpr : public Expr {
public:
    Token token;
    bool isFloat;
    
    NumberExpr(const Token& t, bool isF) : token(t), isFloat(isF) {}
    void accept(Visitor* visitor) override;
};

class StringExpr : public Expr {
public:
    Token token;
    
    explicit StringExpr(const Token& t) : token(t) {}
    void accept(Visitor* visitor) override;
};

class BoolExpr : public Expr {
public:
    Token token;
    bool value;
    
    BoolExpr(const Token& t, bool v) : token(t), value(v) {}
    void accept(Visitor* visitor) override;
};

// Variable reference
class VariableExpr : public Expr {
public:
    Token name;
    
    explicit VariableExpr(const Token& n) : name(n) {}
    void accept(Visitor* visitor) override;
};

// Array access expression
class ArrayAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    
    ArrayAccessExpr(std::unique_ptr<Expr> arr, std::unique_ptr<Expr> idx)
        : array(std::move(arr)), index(std::move(idx)) {}
    void accept(Visitor* visitor) override;
};

// Binary expression
class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;
    
    BinaryExpr(std::unique_ptr<Expr> l, const Token& o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    void accept(Visitor* visitor) override;
};

// Unary expression
class UnaryExpr : public Expr {
public:
    Token op;
    std::unique_ptr<Expr> expr;
    
    UnaryExpr(const Token& o, std::unique_ptr<Expr> e)
        : op(o), expr(std::move(e)) {}
    void accept(Visitor* visitor) override;
};

class TypeExpr : public Expr {
public:
    Type type;
    
    explicit TypeExpr(const Type& t) : type(t) {}
    void accept(Visitor* visitor) override;
};

// Assignment expression
class AssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;  // VariableExpr or ArrayAccessExpr
    Token op;  // = += -= *= /=
    std::unique_ptr<Expr> value;
    
    AssignExpr(std::unique_ptr<Expr> t, const Token& o, std::unique_ptr<Expr> v)
        : target(std::move(t)), op(o), value(std::move(v)) {}
    void accept(Visitor* visitor) override;
};

// Function call
class CallExpr : public Expr {
public:
    Token name;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(const Token& n, std::vector<std::unique_ptr<Expr>> args)
        : name(n), arguments(std::move(args)) {}
    void accept(Visitor* visitor) override;
};

// Array initialization
class ArrayInitExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    
    explicit ArrayInitExpr(std::vector<std::unique_ptr<Expr>> elems)
        : elements(std::move(elems)) {}
    void accept(Visitor* visitor) override;
};

// Array allocation
class ArrayAllocExpr : public Expr {
public:
    Type elementType;
    std::unique_ptr<Expr> size;
    
    ArrayAllocExpr(const Type& type, std::unique_ptr<Expr> s)
        : elementType(type), size(std::move(s)) {}
    void accept(Visitor* visitor) override;
};

// Statement nodes
class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    
    explicit ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
    void accept(Visitor* visitor) override;
};

class VarDeclStmt : public Stmt {
public:
    Token name;
    Type type;
    std::unique_ptr<Expr> initializer;
    
    VarDeclStmt(const Token& n, const Type& t, std::unique_ptr<Expr> init = nullptr)
        : name(n), type(t), initializer(std::move(init)) {}
    void accept(Visitor* visitor) override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)) {}
    void accept(Visitor* visitor) override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenB,
           std::unique_ptr<Stmt> elseB = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}
    void accept(Visitor* visitor) override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    void accept(Visitor* visitor) override;
};

class ReturnStmt : public Stmt {
public:
    Token keyword;
    std::unique_ptr<Expr> value;
    
    ReturnStmt(const Token& kw, std::unique_ptr<Expr> val = nullptr)
        : keyword(kw), value(std::move(val)) {}
    void accept(Visitor* visitor) override;
};

// Function parameter
struct Parameter {
    Token name;
    Type type;
    
    Parameter(const Token& n, const Type& t) : name(n), type(t) {}
};

// Function declaration
class FunctionDecl : public ASTNode {
public:
    Token name;
    Type returnType;
    std::vector<Parameter> parameters;
    std::unique_ptr<BlockStmt> body;
    
    FunctionDecl(const Token& n, const Type& rt,
                std::vector<Parameter> params,
                std::unique_ptr<BlockStmt> b)
        : name(n), returnType(rt), parameters(std::move(params)),
          body(std::move(b)) {}
    void accept(Visitor* visitor) override;
};

// Program node (top-level)
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    
    explicit Program(std::vector<std::unique_ptr<FunctionDecl>> funcs)
        : functions(std::move(funcs)) {}
    void accept(Visitor* visitor) override;
};


#endif // AST_H