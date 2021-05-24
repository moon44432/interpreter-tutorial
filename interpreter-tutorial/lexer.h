
// MicroSEL
// lexer.h

#pragma once

#include <string>
#include "value.h"

typedef enum Token
{
    tok_eof = -1,

    // commands
    tok_func = -2,

    // primary
    tok_identifier = -10,
    tok_number = -11,

    // control
    tok_if = -16,
    tok_then = -17,
    tok_else = -18,
    tok_for = -19,
    tok_while = -20,
    tok_break = -30,
    tok_return = -31,

    // array declaration
    tok_arr = -46,
    tok_as = -47,

    // block and bracket
    tok_openblock = -90,
    tok_closeblock = -91,
};

extern std::string IdStr;
extern std::string MainCode;

extern double NumVal;
extern int LastChar;

int GetTok(const std::string& Code, int& Idx);