
// MicroSEL
// parser.cpp
    
#include "stdfunc.h"
#include "execute.h"
#include "value.h"

std::vector<std::string> StdFuncList = {
    "print",
    "println",
    "printch",
    "input",
    "inputch",
};

Value CallStdFunc(const std::string& Name, const std::vector<Value>& Args)
{
    if (Name == "print") return print(Args);
    else if (Name == "println") return println(Args);
    else if (Name == "printch") return println(Args);
    else if (Name == "input") return input(Args);
    else if (Name == "inputch") return inputch(Args);

    return Value(val_err);
}

Value print(const std::vector<Value>& Args)
{
    for (auto Arg : Args)
        fprintf(stderr, "%f ", Arg.getNum());

    return Value(0);
}

Value println(const std::vector<Value>& Args)
{
    for (auto Arg : Args)
        fprintf(stderr, "%f ", Arg.getNum());

    fprintf(stderr, "\n");
    return Value(0);
}

Value printch(const std::vector<Value>& Args)
{
    for (auto Arg : Args)
        fprintf(stderr, "%c", (char)Arg.getNum());

    fprintf(stderr, "\n");
    return Value(0);
}

Value input(const std::vector<Value>& Args)
{
    if (Args.size() != 0) return LogErrorV("input() requires no arguments");

    double Val;
    fscanf(stdin, "%lf", &Val);

    if (trunc(Val) == Val) return Value((int)Val);
    else return Value(Val);
}

Value inputch(const std::vector<Value>& Args)
{
    if (Args.size() != 0) return LogErrorV("inputch() requires no arguments");

    char Val;
    fscanf(stdin, "%c", &Val);
    return Value(Val);
}