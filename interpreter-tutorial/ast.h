
// MicroSEL
// ast.h

#pragma once

#include "value.h"
#include <string>
#include <vector>
#include <memory>

typedef enum NodeType
{
    node_default = 0, // 기타 모든 노드
    node_var = 1, // 변수 참조 노드
    node_deref = 2, // 역참조 노드
} nodeType;

/// ExprAST - 구문 트리의 기본 노드
class ExprAST
{
    int NodeType = node_default;
public:
    int getNodeType() { return NodeType; }
    void setNodeType(int Type) { NodeType = Type; }
    virtual ~ExprAST() = default;
    virtual Value execute() = 0;
};

/// NumberExprAST - "1.0"과 같은 숫자 리터럴 표현.
class NumberExprAST : public ExprAST
{
    Value Val;

public:
    NumberExprAST(Value Val) : Val(Val) {}
    Value execute() override;
};

/// VariableExprAST - "i"나 "ar[2][3]"와 같이 변수나 배열 요소를 참조하는 표현.
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

/// DeRefExprAST - "@a"나 "@(ptr + 10)"와 같이 메모리 주소를 역참조하는 표현.
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

/// ArrDeclExprAST - "arr ar[2][2][2]"와 같이 배열을 선언하는 표현.
class ArrDeclExprAST : public ExprAST
{
    std::string Name;
    std::vector<int> Indices;

public:
    ArrDeclExprAST(std::string Name, std::vector<int> Indices) : Name(Name), Indices(std::move(Indices)) {}
    Value execute() override;
};

/// UnaryExprAST - 단항 연산 표현.
class UnaryExprAST : public ExprAST
{
    char Opcode;
    std::shared_ptr<ExprAST> Operand;

public:
    UnaryExprAST(char Opcode, std::shared_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}
    Value execute() override;
};

/// BinaryExprAST - 이항 연산 표현.
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

/// CallExprAST - 함수 호출 표현.
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

/// IfExprAST - if/then/else 조건문 표현.
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

/// ForExprAST - for 제어문 표현.
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

/// WhileExprAST - while 제어문 표현.
class WhileExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> Cond, Body;

public:
    WhileExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Body)
        : Cond(std::move(Cond)), Body(std::move(Body)) {}
    Value execute() override;
};

/// BlockExprAST - 블록으로 묶인 연속된 표현식 표현.
class BlockExprAST : public ExprAST
{
    std::vector<std::shared_ptr<ExprAST>> Expressions;

public:
    BlockExprAST(std::vector<std::shared_ptr<ExprAST>> Expressions)
        : Expressions(std::move(Expressions)) {}
    Value execute() override;
};

/// BreakExprAST - 반복문 탈출 표현.
class BreakExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> Expr;

public:
    BreakExprAST(std::shared_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value execute() override;
};

/// ReturnExprAST - 함수의 리턴 표현.
class ReturnExprAST : public ExprAST
{
    std::shared_ptr<ExprAST> Expr;

public:
    ReturnExprAST(std::shared_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    Value execute() override;
};

/// PrototypeAST - 함수의 프로토타입
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

/// PrototypeAST - 함수의 본체
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