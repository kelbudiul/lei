// ast.h
#ifndef AST_H
#define AST_H

#include "../include/common.h"
#include "visitor.h"

struct ASTNode : Visitable {
    virtual ~ASTNode() = default;
    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct ExpressionAST : ASTNode {
    virtual Type getType() const = 0;
};

struct StatementAST : ASTNode {};

struct BlockAST : StatementAST {
    std::vector<std::unique_ptr<StatementAST>> statements;
    
    BlockAST() = default;
    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct FunctionAST : ASTNode {
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::unique_ptr<BlockAST> body;

    FunctionAST(const std::string& name, 
                const std::string& returnType,
                std::vector<std::pair<std::string, std::string>> params,
                std::unique_ptr<BlockAST> body);

    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct VariableDeclarationAST : StatementAST {
    std::string name;
    std::string type;
    std::unique_ptr<ExpressionAST> initializer;

    VariableDeclarationAST(const std::string& name, 
                          const std::string& type,
                          std::unique_ptr<ExpressionAST> init = nullptr);

    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct AssignmentAST : StatementAST {
    std::string name;
    std::unique_ptr<ExpressionAST> value;
    std::string op; // "=", "+=", "-=", "*=", "/="

    AssignmentAST(const std::string& name,
                 std::unique_ptr<ExpressionAST> value,
                 const std::string& op = "=");

    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct IfStatementAST : StatementAST {
    std::unique_ptr<ExpressionAST> condition;
    std::unique_ptr<BlockAST> thenBlock;
    std::unique_ptr<BlockAST> elseBlock;

    IfStatementAST(std::unique_ptr<ExpressionAST> cond,
                   std::unique_ptr<BlockAST> thenB,
                   std::unique_ptr<BlockAST> elseB = nullptr);

    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct WhileStatementAST : StatementAST {
    std::unique_ptr<ExpressionAST> condition;
    std::unique_ptr<BlockAST> body;

    WhileStatementAST(std::unique_ptr<ExpressionAST> cond,
                      std::unique_ptr<BlockAST> body);

    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct ReturnAST : StatementAST {
    std::unique_ptr<ExpressionAST> value;

    ReturnAST(std::unique_ptr<ExpressionAST> value);
    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct LiteralAST : ExpressionAST {
    std::variant<int, float, bool, std::string> value;
    Type type;

    LiteralAST(const std::variant<int, float, bool, std::string>& val);
    Type getType() const override { return type; }
    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct VariableDeclarationAST : StatementAST {
    std::string name;
    std::string type;
    std::unique_ptr<ExpressionAST> initializer;  // Can be nullptr for uninitialized variables

    VariableDeclarationAST(const std::string& name, 
                          const std::string& type,
                          std::unique_ptr<ExpressionAST> init = nullptr)
        : name(name), type(type), initializer(std::move(init)) {}

    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct CallExprAST : ExpressionAST {
    std::string callee;
    std::vector<std::unique_ptr<ExpressionAST>> arguments;
    Type type;

    CallExprAST(const std::string& callee,
                std::vector<std::unique_ptr<ExpressionAST>> args);
    Type getType() const override { return type; }
    void accept(Visitor* visitor) override { visitor->visit(this); }
};


struct BinaryExprAST : ExpressionAST {
    enum class Op {
        Add, Sub, Mul, Div,
        Eq, NotEq, Less, LessEq, Greater, GreaterEq,
        And, Or
    };
    
    Op op;
    std::unique_ptr<ExpressionAST> left;
    std::unique_ptr<ExpressionAST> right;
    
    BinaryExprAST(Op op, 
                  std::unique_ptr<ExpressionAST> left,
                  std::unique_ptr<ExpressionAST> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}
    
    Type getType() const override {
        switch (op) {
            case Op::Eq:
            case Op::NotEq:
            case Op::Less:
            case Op::LessEq:
            case Op::Greater:
            case Op::GreaterEq:
            case Op::And:
            case Op::Or:
                return Type::Bool;
            default:
                return left->getType(); // Arithmetic ops preserve type
        }
    }
    
    void accept(Visitor* visitor) override { visitor->visit(this); }
};

struct UnaryExprAST : ExpressionAST {
    enum class Op {
        Neg,  // -
        Not   // !
    };
    
    Op op;
    std::unique_ptr<ExpressionAST> operand;
    
    UnaryExprAST(Op op, std::unique_ptr<ExpressionAST> operand)
        : op(op), operand(std::move(operand)) {}
        
    Type getType() const override {
        return op == Op::Not ? Type::Bool : operand->getType();
    }
    
    void accept(Visitor* visitor) override { visitor->visit(this); }
};

#endif // AST_H