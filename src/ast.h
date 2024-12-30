#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>
#include <string>
#include "token.h"
#include "visitor.h"

// Location information for AST nodes
struct Location {
    int line;
    int column;
    
    Location(int l = 0, int c = 0) : line(l), column(c) {}
    Location(const Token& token) : line(token.line), column(token.column) {}
};

// Type representation
struct Type {
    std::string name;
    bool isArray;
    int arraySize;  // -1 for dynamic arrays
    
    Type(const std::string& n, bool arr = false, int size = -1)
        : name(n), isArray(arr), arraySize(size) {}
        
    bool isDynamicArray() const { return isArray && arraySize < 0; }
    bool isFixedArray() const { return isArray && arraySize >= 0; }
};

// Base AST node class
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(Visitor* visitor) = 0;
    
    Location loc;
    ASTNode* parent = nullptr;  // Parent pointer
    
    void setParent(ASTNode* p) { parent = p; }
    
protected:
    explicit ASTNode(const Location& location) : loc(location) {}
};

// Expression node base class
class Expr : public ASTNode {
public:
    explicit Expr(const Location& location) : ASTNode(location) {}
    virtual ~Expr() = default;
};

// Statement node base class
class Stmt : public ASTNode {
public:
    explicit Stmt(const Location& location) : ASTNode(location) {}
    virtual ~Stmt() = default;
};

// Literal expression nodes
class NumberExpr : public Expr {
public:
    Token token;
    bool isFloat;
    
    NumberExpr(const Token& t, bool isF)
        : Expr(Location(t)), token(t), isFloat(isF) {}
    void accept(Visitor* visitor) override;
};

class StringExpr : public Expr {
public:
    Token token;
    
    explicit StringExpr(const Token& t)
        : Expr(Location(t)), token(t) {}
    void accept(Visitor* visitor) override;
};

class BoolExpr : public Expr {
public:
    Token token;
    bool value;
    
    BoolExpr(const Token& t, bool v)
        : Expr(Location(t)), token(t), value(v) {}
    void accept(Visitor* visitor) override;
};

class VariableExpr : public Expr {
public:
    Token name;
    
    explicit VariableExpr(const Token& n)
        : Expr(Location(n)), name(n) {}
    void accept(Visitor* visitor) override;
};

class ArrayAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    
    ArrayAccessExpr(std::unique_ptr<Expr> arr, std::unique_ptr<Expr> idx, const Token& bracket)
        : Expr(Location(bracket)), array(std::move(arr)), index(std::move(idx)) {}
    void accept(Visitor* visitor) override;
};

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;
    
    BinaryExpr(std::unique_ptr<Expr> l, const Token& o, std::unique_ptr<Expr> r)
        : Expr(Location(o)), left(std::move(l)), op(o), right(std::move(r)) {}
    void accept(Visitor* visitor) override;
};

class UnaryExpr : public Expr {
public:
    Token op;
    std::unique_ptr<Expr> expr;
    
    UnaryExpr(const Token& o, std::unique_ptr<Expr> e)
        : Expr(Location(o)), op(o), expr(std::move(e)) {}
    void accept(Visitor* visitor) override;
};

class TypeExpr : public Expr {
public:
    Type type;
    
    TypeExpr(const Type& t, const Token& typeToken)
        : Expr(Location(typeToken)), type(t) {}
    void accept(Visitor* visitor) override;
};

class AssignExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    Token op;
    std::unique_ptr<Expr> value;
    
    AssignExpr(std::unique_ptr<Expr> t, const Token& o, std::unique_ptr<Expr> v)
        : Expr(Location(o)), target(std::move(t)), op(o), value(std::move(v)) {}
    void accept(Visitor* visitor) override;
};

class CallExpr : public Expr {
public:
    Token name;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(const Token& n, std::vector<std::unique_ptr<Expr>> args)
        : Expr(Location(n)), name(n), arguments(std::move(args)) {}
    void accept(Visitor* visitor) override;
};

class ArrayInitExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    size_t inferredSize;    
    ArrayInitExpr(std::vector<std::unique_ptr<Expr>> elems, const Token& braceToken)
        : Expr(Location(braceToken)), elements(std::move(elems)), inferredSize(elements.size()) {}
    void accept(Visitor* visitor) override;
};

class ArrayAllocExpr : public Expr {
public:
    Type elementType;
    std::unique_ptr<Expr> size;
    
    ArrayAllocExpr(const Type& type, std::unique_ptr<Expr> s, const Token& newToken)
        : Expr(Location(newToken)), elementType(type), size(std::move(s)) {}
    void accept(Visitor* visitor) override;
};

// Statement nodes
class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    
    ExprStmt(std::unique_ptr<Expr> e, const Token& startToken)
        : Stmt(Location(startToken)), expr(std::move(e)) {}
    void accept(Visitor* visitor) override;
};

class VarDeclStmt : public Stmt {
public:
    Token name;
    Type type;
    std::unique_ptr<Expr> initializer;
    
    VarDeclStmt(const Token& n, const Type& t, std::unique_ptr<Expr> init, const Token& varToken)
        : Stmt(Location(varToken)), name(n), type(t), initializer(std::move(init)) {}
    void accept(Visitor* visitor) override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts, const Token& braceToken)
        : Stmt(Location(braceToken)), statements(std::move(stmts)) {}
        
    void accept(Visitor* visitor) override;
    void addStatement(std::unique_ptr<Stmt> stmt);
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenB,
           std::unique_ptr<Stmt> elseB, const Token& ifToken)
        : Stmt(Location(ifToken)), condition(std::move(cond)),
          thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
    void accept(Visitor* visitor) override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b, const Token& whileToken)
        : Stmt(Location(whileToken)), condition(std::move(cond)), body(std::move(b)) {}
    void accept(Visitor* visitor) override;
};

class ReturnStmt : public Stmt {
public:
    Token keyword;
    std::unique_ptr<Expr> value;
    
    ReturnStmt(const Token& kw, std::unique_ptr<Expr> val)
        : Stmt(Location(kw)), keyword(kw), value(std::move(val)) {}
    void accept(Visitor* visitor) override;
};

// Function parameter
struct Parameter {
    Token name;
    Type type;
    
    Parameter(const Token& n, const Type& t)
        : name(n), type(t) {}
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
                std::unique_ptr<BlockStmt> b,
                const Token& fnToken)
        : ASTNode(Location(fnToken)), name(n), returnType(rt),
          parameters(std::move(params)), body(std::move(b)) {}
    void accept(Visitor* visitor) override;
    void setBody(std::unique_ptr<BlockStmt> b);

};

// Program node (top-level)
class Program : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    
    Program(std::vector<std::unique_ptr<FunctionDecl>> funcs, const Token& startToken)
        : ASTNode(Location(startToken)), functions(std::move(funcs)) {}
    void accept(Visitor* visitor) override;
};

#endif // AST_H