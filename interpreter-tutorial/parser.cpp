
// MicroSEL
// parser.cpp

#include "lexer.h"
#include "ast.h"
#include <cstdio>
#include <map>

int CurTok;
std::map<std::string, int> BinopPrecedence;
std::string OpChrList = "<>+-*/%!&|=";

void InitBinopPrec()
{
    BinopPrecedence["**"] = 18 - 4; // 가장 높은 우선순위
    BinopPrecedence["*"] = 18 - 5;
    BinopPrecedence["/"] = 18 - 5;
    BinopPrecedence["%"] = 18 - 5;
    BinopPrecedence["+"] = 18 - 6;
    BinopPrecedence["-"] = 18 - 6;
    BinopPrecedence["<"] = 18 - 8;
    BinopPrecedence[">"] = 18 - 8;
    BinopPrecedence["<="] = 18 - 8;
    BinopPrecedence[">="] = 18 - 8;
    BinopPrecedence["=="] = 18 - 9;
    BinopPrecedence["!="] = 18 - 9;
    BinopPrecedence["&&"] = 18 - 13;
    BinopPrecedence["||"] = 18 - 14;
    BinopPrecedence["="] = 18 - 15; // 가장 낮은 우선순위
}

int GetNextToken(const std::string& Code, int& Idx)
{
    return CurTok = GetTok(Code, Idx);
}

/// GetPrecedence - 이항 연산자의 우선순위를 얻는다.
int GetPrecedence(std::string Op)
{
    int TokPrec = BinopPrecedence[Op];

    if (TokPrec <= 0) return -1;
    return TokPrec;
}

/// LogError* - 에러 핸들링 함수들.
std::shared_ptr<ExprAST> LogError(const char* Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::shared_ptr<PrototypeAST> LogErrorP(const char* Str)
{
    LogError(Str);
    return nullptr;
}

/// numberexpr ::= number
std::shared_ptr<ExprAST> ParseNumberExpr(const std::string& Code, int& Idx)
{
    // 함수 호출 시점에서 현재 토큰은 숫자.
    GetNextToken(Code, Idx); // 현재 토큰을 소비하고 다음 토큰을 미리 받아둔다.

    return std::make_shared<NumberExprAST>(Value(NumVal));
}

/// parenexpr ::= '(' expression ')'
std::shared_ptr<ExprAST> ParseParenExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat '('.

    auto Expr = ParseExpression(Code, Idx);
    if (!Expr) return nullptr;

    if (CurTok != ')')
        return LogError("Expected ')'");

    GetNextToken(Code, Idx); // eat ')'.

    return Expr;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier ('[' expression ']')+
///   ::= identifier '(' expression* ')'
std::shared_ptr<ExprAST> ParseIdentifierExpr(const std::string& Code, int& Idx)
{
    std::string IdName = IdStr;

    GetNextToken(Code, Idx); // eat identifier.

    // 식별자 뒤에 '('가 있으면 함수 호출로 취급.
    // 그렇지 않으면 변수나 배열 원소 참조로 취급.
    if (CurTok != '(')
    {
        // 변수 참조
        if (CurTok != '[')
            return std::make_shared<VariableExprAST>(IdName);

        // 배열 원소 참조
        GetNextToken(Code, Idx); // eat '['.

        std::vector<std::shared_ptr<ExprAST>> Indices;
        if (CurTok != ']')
        {
            while (true)
            {
                if (auto ArrIdx = ParseExpression(Code, Idx))
                    Indices.push_back(std::move(ArrIdx));
                else return nullptr;

                if (CurTok == ']')
                {
                    GetNextToken(Code, Idx); // eat ']'.

                    if (CurTok != '[') break; // 인덱스가 더 이상 주어지지 않는 경우
                    else GetNextToken(Code, Idx); // 인덱스가 계속 주어지는 경우
                }
            }
        }
        else return LogError("Array index missing");

        return std::make_shared<VariableExprAST>(IdName, std::move(Indices));
    }

    // 함수를 호출하는 경우
    GetNextToken(Code, Idx); // eat '('.

    std::vector<std::shared_ptr<ExprAST>> Args;
    if (CurTok != ')')
    {
        while (true)
        {
            if (auto Arg = ParseExpression(Code, Idx))
                Args.push_back(std::move(Arg));
            else return nullptr;

            if (CurTok == ')') break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");

            GetNextToken(Code, Idx);
        }
    }
    // eat ')'.
    GetNextToken(Code, Idx);

    return std::make_shared<CallExprAST>(IdName, std::move(Args));
}

/// derefexpr
///   ::= '@' expression
std::shared_ptr<ExprAST> ParseDeRefExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat '@'.

    auto Primary = ParsePrimary(Code, Idx);
    if (!Primary) return nullptr;

    return std::make_shared<DeRefExprAST>(std::move(Primary));
}

/// arrdeclexpr ::= 'arr' identifier ('[' number ']')+
std::shared_ptr<ExprAST> ParseArrDeclExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat "arr".

    std::string IdName = IdStr;
    GetNextToken(Code, Idx); // eat identifier string.

    if (CurTok != '[') return LogError("Expected '[' after array name");

    GetNextToken(Code, Idx); // eat '['.

    std::vector<int> Indices;
    if (CurTok != ']')
    {
        while (true)
        {
            // 배열의 각 차원의 너비는 1 이상의 정수이어야 함.
            // 값이 (비록 내부적으로는 실수형으로 저장되지만) 1 이상의 정수인지 체크함.
            if (CurTok == tok_number && trunc(NumVal) == NumVal && (int)NumVal >= 1)
                Indices.push_back((int)NumVal);
            else return LogError("Length of each dimension must be an integer 1 or higher");
            
            GetNextToken(Code, Idx);

            if (CurTok == ']')
            {
                if (LastChar != '[') break; // 차원 정보가 더 이상 주어지지 않는 경우

                GetNextToken(Code, Idx); // eat ']'.
                GetNextToken(Code, Idx); // eat '['.
            }
        }
    }
    else return LogError("Array dimension missing");

    GetNextToken(Code, Idx);

    return std::make_shared<ArrDeclExprAST>(IdName, std::move(Indices));
}

/// ifexpr ::= 'if' expression 'then' blockexpr 'else' blockexpr
std::shared_ptr<ExprAST> ParseIfExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat "if".

    auto Cond = ParseExpression(Code, Idx);
    if (!Cond) return nullptr;

    if (CurTok != tok_then)
        return LogError("Expected then");

    GetNextToken(Code, Idx); // eat "then".

    auto Then = ParseBlockExpression(Code, Idx);
    if (!Then) return nullptr;

    // else expression is optional.
    if (CurTok == tok_else)
    {
        GetNextToken(Code, Idx); // eat "else".

        auto Else = ParseBlockExpression(Code, Idx);
        if (!Else) return nullptr;

        return std::make_shared<IfExprAST>(std::move(Cond), std::move(Then),
            std::move(Else));
    }
    return std::make_shared<IfExprAST>(std::move(Cond), std::move(Then));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? blockexpr
std::shared_ptr<ExprAST> ParseForExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat "for".

    if (CurTok != tok_identifier)
        return LogError("Expected identifier");

    std::string IdName = IdStr;
    GetNextToken(Code, Idx); // eat identifier string.

    if (CurTok != '=')
        return LogError("Expected '=' after identifier");
    GetNextToken(Code, Idx); // eat '='.

    auto Start = ParseExpression(Code, Idx);
    if (!Start) return nullptr;

    if (CurTok != ',')
        return LogError("Expected ','");
    GetNextToken(Code, Idx);

    auto End = ParseExpression(Code, Idx);
    if (!End) return nullptr;

    // step value is optional.
    std::shared_ptr<ExprAST> Step;
    if (CurTok == ',')
    {
        GetNextToken(Code, Idx);

        Step = ParseExpression(Code, Idx);
        if (!Step) return nullptr;
    }

    auto Body = ParseBlockExpression(Code, Idx);
    if (!Body) return nullptr;

    return std::make_shared<ForExprAST>(IdName, std::move(Start), std::move(End),
        std::move(Step), std::move(Body));
}

/// whileexpr ::= 'while' expr blockexpr
std::shared_ptr<ExprAST> ParseWhileExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat "while".

    auto Cond = ParseExpression(Code, Idx);
    if (!Cond) return nullptr;

    auto Body = ParseBlockExpression(Code, Idx);
    if (!Body) return nullptr;

    return std::make_shared<WhileExprAST>(std::move(Cond), std::move(Body));
}

/// breakexpr
///   ::= 'break' expr
std::shared_ptr<ExprAST> ParseBreakExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat "break".

    auto Expr = ParseExpression(Code, Idx);
    if (!Expr) return nullptr;

    return std::make_shared<BreakExprAST>(std::move(Expr));
}

/// returnexpr
///   ::= 'return' expr
std::shared_ptr<ExprAST> ParseReturnExpr(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat "return".

    auto Expr = ParseExpression(Code, Idx);
    if (!Expr) return nullptr;

    return std::make_shared<ReturnExprAST>(std::move(Expr));
}

/// primary
///   ::= identifierexpr
///   ::= derefexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= whileexpr
///   ::= reptexpr
///   ::= loopexpr
std::shared_ptr<ExprAST> ParsePrimary(const std::string& Code, int& Idx)
{
    switch (CurTok)
    {
    case tok_identifier:
        return ParseIdentifierExpr(Code, Idx);
    case '@':
        return ParseDeRefExpr(Code, Idx);
    case tok_number:
        return ParseNumberExpr(Code, Idx);
    case tok_for:
        return ParseForExpr(Code, Idx);
    case tok_while:
        return ParseWhileExpr(Code, Idx);
    case tok_if:
        return ParseIfExpr(Code, Idx);
    case tok_openbrkt:
        return ParseParenExpr(Code, Idx);
    default:
        return LogError("Unknown token when expecting an expression");
    }
}

/// unary
///   ::= primary
///   ::= unaryop unary
std::shared_ptr<ExprAST> ParseUnary(const std::string& Code, int& Idx)
{
    // If the current token is not an operator, it must be a primary expr.
    if (!isascii(CurTok) || CurTok == '(' || CurTok == ',' || CurTok == '@')
        return ParsePrimary(Code, Idx);

    // If this is a unary operator, read it.
    int Opc;
    if (OpChrList.find(CurTok) != std::string::npos)
    {
        Opc = CurTok;
        GetNextToken(Code, Idx);
    }
    else return LogError(((std::string)"Unknown token '" + (char)CurTok + (std::string)"'").c_str());

    if (auto Operand = ParseUnary(Code, Idx))
        return std::make_shared<UnaryExprAST>(Opc, std::move(Operand));
    return nullptr;
}

/// binoprhs
///   ::= (binop unary)*
std::shared_ptr<ExprAST> ParseBinOpRHS(const std::string& Code, int& Idx, int ExprPrec, std::shared_ptr<ExprAST> LHS)
{
    while (true)
    {
        bool DoubleCh = false;
        std::string BinOp;
        BinOp += (char)CurTok;

        // 이항 연산자가 문자 두 개로 이루어진 경우
        if (OpChrList.find(CurTok) != std::string::npos &&
            OpChrList.find(LastChar) != std::string::npos)
        {
            BinOp += (char)LastChar;
            DoubleCh = true;
        }
        int TokPrec = GetPrecedence(BinOp); // 이항 연산자의 우선순위 얻기

        // 새롭게 얻은 이항 연산자(BinOp)가 첫 번째로 파싱된 이항 연산자이거나,
        // 우선순위가 이전의 이항 연산자보다 높은 경우 계속 진행함.
        // 그렇지 않으면 LHS를 리턴하고 탈출함.
        if (TokPrec <= ExprPrec) return LHS;

        GetNextToken(Code, Idx);
        if (DoubleCh) GetNextToken(Code, Idx);

        // 이항 연산자 우측에 위치하는 단항식 분석
        auto RHS = ParseUnary(Code, Idx);
        if (!RHS) return nullptr;

        // 이제 식이 어떠한 형태로 묶일지 결정되어야 한다.
        // 예컨대, (a + b) binop rhs 와 a + (b binop rhs) 중 하나이어야 한다.
        // 따라서 우측 단항식 다음에 올 이항 연산자 NextOp를 미리 확인하여
        // 우선 순위를 비교해야 한다.
        
        std::string NextOp;
        NextOp += (char)CurTok;

        if (OpChrList.find(CurTok) != std::string::npos &&
            OpChrList.find(LastChar) != std::string::npos)
        {
            NextOp += (char)LastChar;
        }

        int NextPrec = GetPrecedence(NextOp);

        // NextOp가 BinOp의 우선 순위보다 높은 경우,
        // 식은 a + (b binop rhs) 와 같은 형태로 묶여야 한다. ex) a + (b * rhs)
        if (TokPrec < NextPrec)
        {
            RHS = ParseBinOpRHS(Code, Idx, TokPrec, std::move(RHS));
            if (!RHS) return nullptr;
        }

        // NextOp가 BinOp의 우선 순위보다 낮은 경우,
        // 식은 (a + b) binop rhs 와 같은 형태로 묶여야 한다.
        // LHS와 RHS를 병합하고 계속 파싱한다. ex) (a + b) || rhs
        LHS = std::make_shared<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

/// expression
///   ::= unary binoprhs
///   ::= arrdeclexpr
///   ::= breakexpr
///   ::= returnexpr
std::shared_ptr<ExprAST> ParseExpression(const std::string& Code, int& Idx)
{
    switch (CurTok)
    {
    case tok_arr:
        return ParseArrDeclExpr(Code, Idx);
    case tok_break:
        return ParseBreakExpr(Code, Idx);
    case tok_return:
        return ParseReturnExpr(Code, Idx);
    default:
        auto LHS = ParseUnary(Code, Idx);
        if (!LHS) return nullptr;
        return ParseBinOpRHS(Code, Idx, 0, std::move(LHS));
    }
}

/// blockexpr
///   ::= expression
///   ::= '{' expression+ '}'
std::shared_ptr<ExprAST> ParseBlockExpression(const std::string& Code, int& Idx)
{
    if (CurTok != tok_openblock)
        return ParseExpression(Code, Idx);
    GetNextToken(Code, Idx);

    std::vector<std::shared_ptr<ExprAST>> ExprSeq;

    while (true)
    {
        auto Expr = ParseBlockExpression(Code, Idx);
        ExprSeq.push_back(std::move(Expr));
        if (CurTok == ';')
            GetNextToken(Code, Idx);
        if (CurTok == tok_closeblock)
        {
            GetNextToken(Code, Idx);
            break;
        }
    }
    auto Block = std::make_shared<BlockExprAST>(ExprSeq);
    return std::move(Block);
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER(LETTER)? number? (id, id)
///   ::= unary LETTER (id)
std::shared_ptr<PrototypeAST> ParsePrototype(const std::string& Code, int& Idx)
{
    std::string FnName;

    unsigned int Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
    unsigned int BinaryPrecedence = 18;

    switch (CurTok) {
    default:
        return LogErrorP("Expected function name in prototype");
    case tok_identifier:
        FnName = IdStr;
        Kind = 0;
        GetNextToken(Code, Idx);
        break;
    case tok_unary:
        GetNextToken(Code, Idx);
        if (!isascii(CurTok))
            return LogErrorP("Expected unary operator");
        FnName = "unary";
        FnName += (char)CurTok;
        Kind = 1;
        GetNextToken(Code, Idx);
        break;
    case tok_binary:
        GetNextToken(Code, Idx);
        if (!isascii(CurTok))
            return LogErrorP("Expected binary operator");

        std::string OpName;
        OpName += (char)CurTok;
        GetNextToken(Code, Idx);
        if (OpChrList.find(CurTok) != std::string::npos)
        {
            OpName += (char)CurTok;
            GetNextToken(Code, Idx);
        }
        Kind = 2;

        // Read the precedence if present.
        if (CurTok == tok_number) {
            if (NumVal < 1 || NumVal > 18)
                return LogErrorP("Invalid precedence: must be 1~18");
            BinaryPrecedence = (unsigned int)NumVal;
            GetNextToken(Code, Idx);
        }

        // install binary operator.
        BinopPrecedence[OpName] = BinaryPrecedence;

        FnName = "binary" + OpName;
        break;
    }

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;

    if (GetNextToken(Code, Idx) != ')')
    {
        while (true)
        {
            if (CurTok == tok_identifier)
                ArgNames.push_back(IdStr);

            GetNextToken(Code, Idx);
            if (CurTok == ')') break;
            if (CurTok != ',')
                return LogErrorP("Expected ',' or ')'");

            GetNextToken(Code, Idx);
        }
    }
    // success.
    GetNextToken(Code, Idx); // eat ')'

    // Verify right number of names for operator.
    if (Kind && ArgNames.size() != Kind)
        return LogErrorP("Invalid number of operands for operator");

    return std::make_shared<PrototypeAST>(FnName, ArgNames, Kind != 0,
        BinaryPrecedence);
}

/// definition ::= 'func' prototype expression
std::shared_ptr<FunctionAST> ParseDefinition(const std::string& Code, int& Idx)
{
    GetNextToken(Code, Idx); // eat func.
    auto Proto = ParsePrototype(Code, Idx);
    if (!Proto)
        return nullptr;

    if (auto BlockExpr = ParseBlockExpression(Code, Idx))
        return std::make_shared<FunctionAST>(std::move(Proto), std::move(BlockExpr));
    return nullptr;
}

/// toplevelexpr ::= expression
std::shared_ptr<FunctionAST> ParseTopLevelExpr(const std::string& Code, int& Idx)
{
    if (auto BlockExpr = ParseBlockExpression(Code, Idx)) {
        // Make an anonymous proto.
        auto Proto = std::make_shared<PrototypeAST>("__anon_expr",
            std::vector<std::string>());
        return std::make_shared<FunctionAST>(std::move(Proto), std::move(BlockExpr));
    }
    return nullptr;
}
