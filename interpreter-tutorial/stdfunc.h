
// MicroSEL
// parser.cpp
    
#pragma once
#pragma warning (disable:4996)

#include "ast.h"
#include "value.h"

extern std::vector<std::string> StdFuncList;

Value CallStdFunc(const std::string& Name, const std::vector<Value>& Args);

Value print(const std::vector<Value>& Args);

Value println(const std::vector<Value>& Args);

Value input(const std::vector<Value>& Args);

Value inputch(const std::vector<Value>& Args);
