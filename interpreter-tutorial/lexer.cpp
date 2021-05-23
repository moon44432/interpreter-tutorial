
// MicroSEL
// lexer.cpp

#include "lexer.h"
#include "execute.h"
#include <iostream>

std::string IdStr; // 식별자의 이름을 담기 위한 문자열
double NumVal; // 숫자를 입력받은 경우, 값을 담기 위한 변수
int LastChar = ' '; // 마지막으로 입력받은 문자

int NextCh(const std::string& Code, int& Idx)
{
    if (IsInteractive) return getchar(); // 대화형 모드이면 키보드를 통해 입력받음
    return Code[Idx++]; // 스크립트 실행 모드이면 코드 문자열로부터 읽어 옴
}

int GetTok(const std::string& Code, int& Idx)
{
    // 공백 문자 건너뛰기
    while (isspace(LastChar)) LastChar = NextCh(Code, Idx);

    if (isalpha(LastChar)) // 식별자: [a-zA-Z][a-zA-Z0-9_]*
    {
        IdStr = LastChar;

        while (true)
        {
            LastChar = NextCh(Code, Idx);
            if (isalnum(LastChar) || LastChar == '_') IdStr += LastChar;
            else break;
        }

        // 예약된 키워드
        if (IdStr == "func")
            return tok_func;
        if (IdStr == "arr")
            return tok_arr;
        if (IdStr == "if")
            return tok_if;
        if (IdStr == "then")
            return tok_then;
        if (IdStr == "else")
            return tok_else;
        if (IdStr == "for")
            return tok_for;
        if (IdStr == "while")
            return tok_while;
        if (IdStr == "break")
            return tok_break;
        if (IdStr == "return")
            return tok_return;

        return tok_identifier; // 예약된 키워드가 아닐 경우
    }

    if (isdigit(LastChar) || LastChar == '.') // 숫자: [0-9.]+
    {
        std::string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = NextCh(Code, Idx);
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#') // 라인 끝까지 주석 처리
    {
        do LastChar = NextCh(Code, Idx);
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return GetTok(Code, Idx);
    }

    if (LastChar == '{') // 블록 열기
    {
        LastChar = NextCh(Code, Idx);
        return tok_openblock;
    }
    if (LastChar == '}') // 블록 닫기
    {
        LastChar = NextCh(Code, Idx);
        return tok_closeblock;
    }
    if (LastChar == '(') // 괄호 열기
    {
        LastChar = NextCh(Code, Idx);
        return tok_openbrkt;
    }
    if (LastChar == ')') // 괄호 닫기
    {
        LastChar = NextCh(Code, Idx);
        return tok_closebrkt;
    }

    // 파일의 끝이면 EOF 토큰 반환
    if (LastChar == EOF)
        return tok_eof;

    // 아무 곳에도 속하지 않을 경우 아스키 코드 값 그대로 반환
    int ThisChar = LastChar;
    LastChar = NextCh(Code, Idx);
    return ThisChar;
}