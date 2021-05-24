
// MicroSEL
// execute.cpp
    
#include "value.h"
#include "lexer.h"
#include "ast.h"
#include "execute.h"
#include "stdfunc.h"
#include <map>
#include <cmath>

static std::map<std::string, std::shared_ptr<FunctionAST>> Functions;
static std::vector<namedValue> SymTbl;
static Memory StackMemory;

extern int CurTok;

std::string MainCode;
int MainIdx = 0;

typedef enum ArrAction
{
    getVal,
    getAddr,
    setVal,
} arrAction;

bool IsInteractive = true; // true for default

Value LogErrorV(const char* Str)
{
    LogError(Str);
    return Value(val_err);
}

Value NumberExprAST::execute()
{
    return Val;
}

Value DeRefExprAST::execute()
{
    Value Address = AddrExpr->execute();
    if (Address.isErr())
        return Value(val_err);
    if (Address.isUInt()) // address should be an uint
        return StackMemory.getValue((unsigned int)Address.getNum());

    return LogErrorV("Address must be an unsigned integer");
}

Value HandleArr(std::string ArrName, const std::vector<std::shared_ptr<ExprAST>>& Indices, arrAction Action, Value Val)
{
    for (int i = SymTbl.size() - 1; i >= 0; i--)
    {
        if (SymTbl[i].Name == ArrName && SymTbl[i].IsArr)
        {
            std::vector<Value> IdxV;
            for (int k = 0, e = Indices.size(); k != e; ++k) {
                IdxV.push_back(Indices[k]->execute());
                if (IdxV.back().isErr()) return LogErrorV("Error while calculating indices");
                if (!IdxV.back().isInt()) return LogErrorV("Index must be an integer");
            }

            if (IdxV.size() != SymTbl[i].DimInfo.size()) return LogErrorV("Dimension mismatch");

            int AddVal = 0;
            for (int l = 0; l < IdxV.size(); l++)
            {
                int MulVal = 1;
                for (int m = l + 1; m < IdxV.size(); m++) MulVal *= SymTbl[i].DimInfo[m];
                AddVal += MulVal * IdxV[l].getNum();
            }
            switch (Action)
            {
            case getVal:
                return StackMemory.getValue((unsigned int)(SymTbl[i].Addr + AddVal));
            case getAddr:
                return Value(SymTbl[i].Addr + AddVal);
            case setVal:
                StackMemory.setValue(SymTbl[i].Addr + AddVal, Val);
                return Val;
            }
        }
    }
    return LogErrorV((((std::string)("\"") + ArrName + (std::string)("\" is not an array"))).c_str());
}

Value HandleArr(std::string ArrName, const std::vector<std::shared_ptr<ExprAST>>& Indices, arrAction Action) { return HandleArr(ArrName, Indices, Action, Value()); }

Value VariableExprAST::execute()
{
    if (!Indices.empty()) // array element
        return HandleArr(Name, Indices, getVal);

    // normal variable
    for (int i = SymTbl.size() - 1; i >= 0; i--)
    {
        if (SymTbl[i].Name == Name && !SymTbl[i].IsArr)
            return StackMemory.getValue(SymTbl[i].Addr);
    }
    return LogErrorV(std::string("Identifier \"" + Name + "\" not found").c_str());
}

Value ArrDeclExprAST::execute()
{
    namedValue Arr = { Name, StackMemory.push(Value(0)), true, Indices };
    SymTbl.push_back(Arr);

    int size = 1;
    for (int i = 0; i < Indices.size(); i++) size *= Indices[i];
    for (int i = 0; i < size - 1; i++) StackMemory.push(Value(0));

    return Value(size);
}

Value UnaryExprAST::execute()
{
    if (Opcode == '&') // reference operator
    {
        if (Operand->getNodeType() != node_var)
            return LogErrorV("Operand of '&' must be a variable");

        VariableExprAST* Op = static_cast<VariableExprAST*>(Operand.get());
        const std::vector<std::shared_ptr<ExprAST>>& Indices = Op->getIndices();
        if (!Indices.empty()) // array element
        {
            return HandleArr(Op->getName(), Indices, getAddr);
        }
        else // normal variable
        {
            for (int i = SymTbl.size() - 1; i >= 0; i--)
            {
                if (SymTbl[i].Name == Op->getName())
                    return Value(SymTbl[i].Addr);
            }
            return LogErrorV(std::string("Variable \"" + Op->getName() + "\" not found").c_str());
        }
    }

    Value OperandV = Operand->execute();
    if (OperandV.isErr())
        return Value(val_err);

    switch (Opcode)
    {
    case '!':
        return Value(!(OperandV.getNum()));
        break;
    case '+':
        return Value(+(OperandV.getNum()));
        break;
    case '-':
        return Value(-(OperandV.getNum()));
        break;
    }
}

Value BinaryExprAST::execute() {
    // Special case '=' because we don't want to emit the LHS as an expression.
    if (Op == "=")
    {
        // execute the RHS.
        Value Val = RHS->execute();

        if (Val.isErr())
            return Value(val_err);

        // Assignment requires the LHS to be an identifier.
        VariableExprAST* LHSE;

        if (LHS->getNodeType() == node_var) LHSE = static_cast<VariableExprAST*>(LHS.get());
        else if (LHS->getNodeType() == node_deref)
        {
            DeRefExprAST* LHSE = static_cast<DeRefExprAST*>(LHS.get());

            // update value at the memory address
            Value Addr = LHSE->getExpr()->execute();
            if (!Addr.isUInt()) return LogErrorV("Address must be an unsigned integer");

            StackMemory.setValue(Addr.getNum(), Val);
            return Val;
        }
        else return LogErrorV("Destination of '=' must be a variable");

        // Look up the name.
        std::vector<std::shared_ptr<ExprAST>> Indices = LHSE->getIndices();
        if (!Indices.empty()) // array element
        {
            return HandleArr(LHSE->getName(), Indices, setVal, Val);
        }
        else // normal variable
        {
            bool found = false;
            for (int i = SymTbl.size() - 1; i >= 0; i--)
            {
                if (SymTbl[i].Name == LHSE->getName())
                {
                    StackMemory.setValue(SymTbl[i].Addr, Val);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                namedValue Var = { LHSE->getName(), StackMemory.push(Val), false };
                SymTbl.push_back(Var);
            }
        }
        return Val;
    }

    Value L = LHS->execute();
    Value R = RHS->execute();

    if (L.isErr() || R.isErr())
        return Value(val_err);

    if (Op == "==")
        return Value((L.getNum() == R.getNum()));

    if (Op == "!=")
        return Value((L.getNum() != R.getNum()));

    if (Op == "&&")
        return Value((L.getNum() && R.getNum()));

    if (Op == "||")
        return Value((L.getNum() || R.getNum()));

    if (Op == "<")
        return Value((L.getNum() < R.getNum()));

    if (Op == ">")
        return Value((L.getNum() > R.getNum()));

    if (Op == "<=")
        return Value((L.getNum() <= R.getNum()));

    if (Op == ">=")
        return Value((L.getNum() >= R.getNum()));

    if (Op == "+")
        return Value((L.getNum() + R.getNum()));

    if (Op == "-")
        return Value((L.getNum() - R.getNum()));

    if (Op == "*")
        return Value((L.getNum() * R.getNum()));

    if (Op == "/")
        return Value((L.getNum() / R.getNum()));

    if (Op == "%")
        return Value(fmod(L.getNum(), R.getNum()));

    if (Op == "**")
        return Value(pow(L.getNum(), R.getNum()));
}

Value CallExprAST::execute()
{
    std::vector<Value> ArgsV;
    for (int i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->execute());
        if (ArgsV.back().isErr())
            return Value(val_err);
    }

    if (std::find(StdFuncList.begin(), StdFuncList.end(), Callee) != StdFuncList.end())
        return Value(CallStdFunc(Callee, ArgsV));

    // Look up the name in the global module table.
    std::shared_ptr<FunctionAST> CalleeF = Functions[Callee];
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->argsSize() != Args.size())
        return LogErrorV("Incorrect number of arguments passed");

    return CalleeF->execute(ArgsV);
}

Value IfExprAST::execute()
{
    Value CondV = CondExpr->execute();
    if (CondV.isErr())
        return Value(val_err);

    if (CondV.getNum())
    {
        int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();
        Value ThenV = ThenExpr->execute();

        StackMemory.deleteScope(StackIdx);
        for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

        if (ThenV.isErr())
            return Value(val_err);
        return ThenV;
    }
    else if (ElseExpr != nullptr)
    {
        int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();
        Value ElseV = ElseExpr->execute();

        StackMemory.deleteScope(StackIdx);
        for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

        if (ElseV.isErr())
            return Value(val_err);
        return ElseV;
    }
    return Value(0);
}

Value ForExprAST::execute()
{
    Value StartVal = Start->execute();
    if (StartVal.isErr())
        return Value(val_err);

    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();
    int StartVarAddr;

    bool found = false;
    for (int i = SymTbl.size() - 1; i >= 0; i--)
    {
        if (SymTbl[i].Name == VarName)
        {
            StackMemory.setValue(i, StartVal);
            StartVarAddr = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        namedValue Var = { VarName, StackMemory.push(StartVal), false };
        SymTbl.push_back(Var);
        StartVarAddr = SymTbl.size() - 1;
    }

    // Emit the step value.
    Value StepVal(1);
    if (Step)
    {
        StepVal = Step->execute();
        if (StepVal.isErr())
            return Value(val_err);
    }

    Value BodyExpr, EndCond;
    while (true)
    {
        EndCond = End->execute();
        if (EndCond.isErr() || !EndCond.getNum()) break;

        BodyExpr = Body->execute();
        if (BodyExpr.isErr()) break;
        if (BodyExpr.getType() == val_break)
        {
            BodyExpr.setType(val_data);
            break;
        }
        StackMemory.setValue(StartVarAddr,
            Value(StackMemory.getValue(StartVarAddr).getNum() + StepVal.getNum()));
    }

    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    if (BodyExpr.isErr() || EndCond.isErr())
        return Value(val_err);

    return BodyExpr;
}

Value WhileExprAST::execute()
{
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    Value BodyExpr, EndCond;
    while (true)
    {
        EndCond = Cond->execute();
        if (EndCond.isErr() || !EndCond.getNum()) break;

        BodyExpr = Body->execute();
        if (BodyExpr.isErr()) break;
        if (BodyExpr.getType() == val_break)
        {
            BodyExpr.setType(val_data);
            break;
        }
    }
    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    if (EndCond.isErr() || BodyExpr.isErr())
        return Value(val_err);

    return BodyExpr;
}

Value BreakExprAST::execute()
{
    Value RetVal = Expr->execute();
    if (RetVal.isErr())
        return LogErrorV("Failed to return a value");

    RetVal.setType(val_break);
    return RetVal;
}

Value ReturnExprAST::execute()
{
    Value RetVal = Expr->execute();
    if (RetVal.isErr())
        return LogErrorV("Failed to return a value");

    RetVal.setType(val_return);
    return RetVal;
}

Value BlockExprAST::execute()
{
    Value RetVal(0);
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    for (auto& Expr : Expressions)
    {
        RetVal = Expr->execute();
        if (RetVal.getType() == val_break || 
            RetVal.getType() == val_return) break;
    }
    StackMemory.deleteScope(StackIdx);
    for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();

    return RetVal;
}

Value FunctionAST::execute(std::vector<Value> Ops)
{
    int StackIdx = StackMemory.getSize(), TblIdx = SymTbl.size();

    auto& Arg = Proto->getArgs();
    for (int i = 0; i < Proto->getArgsSize(); i++)
    {
        namedValue ArgVar = { Arg[i], StackMemory.push(Ops[i]), false };
        SymTbl.push_back(ArgVar);
    }

    Value RetVal = Body->execute();

    if (Proto->getName() != "__anon_expr")
    {
        StackMemory.deleteScope(StackIdx);
        for (int i = SymTbl.size(); i > TblIdx; i--) SymTbl.pop_back();
    }

    if (RetVal.isErr()) return Value(val_err);
    if (RetVal.getType() == val_return) RetVal.setType(val_data);

    return RetVal;
}

void HandleDefinition(std::string& Code, int& Idx)
{
    if (auto FnAST = ParseDefinition(Code, Idx))
    {
        if (IsInteractive) fprintf(stderr, "Read function definition\n");
        Functions[FnAST->getFuncName()] = FnAST;
    }
    else GetNextToken(Code, Idx); // Skip token for error recovery.
}

void HandleTopLevelExpression(std::string& Code, int& Idx)
{
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr(Code, Idx))
    {
        Value RetVal = FnAST->execute(std::vector<Value>());
        if (!RetVal.isErr() && IsInteractive)
        {
            fprintf(stderr, "Evaluated to %f\n", RetVal.getNum());
        }
    }
    else GetNextToken(Code, Idx); // Skip token for error recovery.
}

/// top ::= definition | import | external | expression | ';'
void MainLoop(std::string& Code, int& Idx)
{
    bool tmpFlag = false;
    while (true)
    {
        if (IsInteractive) fprintf(stderr, ">>> ");
        switch (CurTok)
        {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            GetNextToken(Code, Idx);
            break;
        case tok_func:
            HandleDefinition(Code, Idx);
            break;
        default:
            HandleTopLevelExpression(Code, Idx);
            break;
        }
    }
}

void ExecuteScript(const char* FileName)
{
    IsInteractive = false;

    FILE* fp = fopen(FileName, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Unknown file name\n");
        return;
    }
    while (!feof(fp)) MainCode += (char)fgetc(fp);
    MainCode += EOF;
    fclose(fp);

    InitBinopPrec();
    GetNextToken(MainCode, MainIdx);
    MainLoop(MainCode, MainIdx);

    fprintf(stderr, "\nExecution finished.\n");
}