
// MicroSEL
// ast.h

#pragma once

#include "value.h"
#include <string>
#include <vector>
#include <memory>

typedef enum NodeType
{
    node_default = 0, // ��Ÿ ��� ���
    node_var = 1, // ���� ���� ���
    node_deref = 2, // ������ ���
} nodeType;

/// ExprAST - ���� Ʈ���� �⺻ ���
class ExprAST
{
    int NodeType = node_default;
public:
    int getNodeType() { return NodeType; }
    void setNodeType(int Type) { NodeType = Type; }
    virtual ~ExprAST() = default;
    virtual Value execute() = 0;
};

/// NumberExprAST - "1.0"�� ���� ���� ���ͷ� ǥ��.
class NumberExprAST : public ExprAST
{
    Value Val;

public:
    NumberExprAST(Value Val) : Val(Val) {}
    Value execute() override;
};

/// VariableExprAST - "i"�� "ar[2][3]"�� ���� ������ �迭 ��Ҹ� �����ϴ� ǥ��.
class VariableExprAST : public ExprAST
{
    std::string Name;
    std::vector<std::shared_ptr<ExprAST>> Indices;

public:
    VariableExprAST(std::string Name, std::vector<std::shared_ptr<ExprAST>> Indices)
        : Name(Name), Indices(std::move(Indices)) {
        setNodeType(nodeType::node_var);
    }
    VariableExprAST(std::string Name) : Name(Name) {
        setNodeType(nodeType::node_var);
    }
    std::string getName() const { return Name; }
    const std::vector<std::shared_ptr<ExprAST>>& getIndices() const { return Indices; }
    Value execute() override;
};

/// DeRefExprAST - "@a"�� "@(ptr + 10)"�� ���� �޸� �ּҸ� �������ϴ� ǥ��.
class DeRefExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> AddrExpr;

public:
    DeRefExprAST(std::shared_ptr<ExprAST> Addr) : AddrExpr(std::move(Addr)) {
        setNodeType(nodeType::node_deref);
    }
    std::shared_ptr<ExprAST> getExpr() const { return AddrExpr; }
    Value execute() override;
};

/// ArrDeclExprAST - "arr ar[2][2][2]"�� ���� �迭�� �����ϴ� ǥ��.
class ArrDeclExprAST : public ExprAST
{
    std::string Name;
    std::vector<int> Indices;

public:
    ArrDeclExprAST(std::string Name, std::vector<int> Indices) : Name(Name), Indices(std::move(Indices)) {}
    Value execute() override;
};

/// UnaryExprAST - ���� ���� ǥ��.
class UnaryExprAST : public ExprAST
{
    char Opcode;
    std::shared_ptr<ExprAST> Operand;

public:
    UnaryExprAST(char Opcode, std::shared_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}
    Value execute() override;
};

/// BinaryExprAST - ���� ���� ǥ��.
class BinaryExprAST : public ExprAST
{
    std::string Op;
    std::shared_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(std::string Op, std::shared_ptr<ExprAST> LHS,
        std::shared_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    Value execute() override;
};

/// CallExprAST - �Լ� ȣ�� ǥ��.
class CallExprAST : public ExprAST
{
    std::string Callee;
    std::vector<std::shared_ptr<ExprAST>> Args;

public:
    CallExprAST(std::string Callee, 
        std::vector<std::shared_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    Value execute() override;
};

/// IfExprAST - if/then/else ���ǹ� ǥ��.
class IfExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> CondExpr, ThenExpr, ElseExpr;

public:
    IfExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Then)
        : CondExpr(std::move(Cond)), ThenExpr(std::move(Then)) {
        ElseExpr = nullptr;
    }
    IfExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Then,
        std::shared_ptr<ExprAST> Else)
        : IfExprAST(std::move(Cond), std::move(Then)) {
        ElseExpr = std::move(Else);
    }
    Value execute() override;
};

/// ForExprAST - for ��� ǥ��.
class ForExprAST : public ExprAST
{
    std::string VarName;
    std::shared_ptr<ExprAST> Start, End, Step, Body;

public:
    ForExprAST(std::string VarName, std::shared_ptr<ExprAST> Start,
        std::shared_ptr<ExprAST> End, std::shared_ptr<ExprAST> Step,
        std::shared_ptr<ExprAST> Body)
        : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}
    Value execute() override;
};

/// WhileExprAST - while ��� ǥ��.
class WhileExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> Cond, Body;

public:
    WhileExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Body)
        : Cond(std::move(Cond)), Body(std::move(Body)) {}
    Value execute() override;
};

/// BlockExprAST - ������� ���� ���ӵ� ǥ���� ǥ��.
class BlockExprAST : public ExprAST
{
    std::vector<std::shared_ptr<ExprAST>> Expressions;

public:
    BlockExprAST(std::vector<std::shared_ptr<ExprAST>> Expressions)
        : Expressions(std::move(Expressions)) {}
    Value execute() override;
};

/// BreakExprAST - �ݺ��� Ż�� ǥ��.
class BreakExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> Expr;

public:
    BreakExprAST(std::shared_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value execute() override;
};

/// ReturnExprAST - �Լ��� ���� ǥ��.
class ReturnExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> Expr;

public:
    ReturnExprAST(std::shared_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value execute() override;
};

/// PrototypeAST - �Լ��� ������Ÿ��
class PrototypeAST
{
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(std::string Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

    std::string getName() const { return Name; }
    int getArgsSize() const { return Args.size(); }
    const std::vector<std::string>& getArgs() const { return Args; }
};

/// PrototypeAST - �Լ��� ��ü
class FunctionAST
{
    std::shared_ptr<PrototypeAST> Proto;
    std::shared_ptr<ExprAST> Body;

public:
    FunctionAST(std::shared_ptr<PrototypeAST> Proto,
        std::shared_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    Value execute(std::vector<Value> Ops);
    std::string getFuncName() const { return Proto->getName(); }
    const std::vector<std::string>& getFuncArgs() const { return Proto->getArgs(); }
    int argsSize() const { return Proto->getArgsSize(); }
};

void InitBinopPrec();

int GetNextToken(const std::string& Code, int& Idx);

int GetPrecedence(std::string Op);

std::shared_ptr<ExprAST> LogError(const char* Str);

std::shared_ptr<PrototypeAST> LogErrorP(const char* Str);

std::shared_ptr<ExprAST> ParseNumberExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseParenExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseIdentifierExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseDeRefExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseArrDeclExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseIfExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseForExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseWhileExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseBreakExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseReturnExpr(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParsePrimary(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseUnary(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseBinOpRHS(const std::string& Code, int& Idx, int ExprPrec, std::shared_ptr<ExprAST> LHS);

std::shared_ptr<ExprAST> ParseExpression(const std::string& Code, int& Idx);

std::shared_ptr<ExprAST> ParseBlockExpression(const std::string& Code, int& Idx);

std::shared_ptr<PrototypeAST> ParsePrototype(const std::string& Code, int& Idx);

std::shared_ptr<FunctionAST> ParseDefinition(const std::string& Code, int& Idx);

std::shared_ptr<FunctionAST> ParseTopLevelExpr(const std::string& Code, int& Idx);