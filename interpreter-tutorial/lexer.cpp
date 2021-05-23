
// MicroSEL
// lexer.cpp

#include "lexer.h"
#include "execute.h"
#include <iostream>

std::string IdStr; // �ĺ����� �̸��� ��� ���� ���ڿ�
double NumVal; // ���ڸ� �Է¹��� ���, ���� ��� ���� ����
int LastChar = ' '; // ���������� �Է¹��� ����

int NextCh(const std::string& Code, int& Idx)
{
    if (IsInteractive) return getchar(); // ��ȭ�� ����̸� Ű���带 ���� �Է¹���
    return Code[Idx++]; // ��ũ��Ʈ ���� ����̸� �ڵ� ���ڿ��κ��� �о� ��
}

int GetTok(const std::string& Code, int& Idx)
{
    // ���� ���� �ǳʶٱ�
    while (isspace(LastChar)) LastChar = NextCh(Code, Idx);

    if (isalpha(LastChar)) // �ĺ���: [a-zA-Z][a-zA-Z0-9_]*
    {
        IdStr = LastChar;

        while (true)
        {
            LastChar = NextCh(Code, Idx);
            if (isalnum(LastChar) || LastChar == '_') IdStr += LastChar;
            else break;
        }

        // ����� Ű����
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

        return tok_identifier; // ����� Ű���尡 �ƴ� ���
    }

    if (isdigit(LastChar) || LastChar == '.') // ����: [0-9.]+
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

    if (LastChar == '#') // ���� ������ �ּ� ó��
    {
        do LastChar = NextCh(Code, Idx);
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return GetTok(Code, Idx);
    }

    if (LastChar == '{') // ��� ����
    {
        LastChar = NextCh(Code, Idx);
        return tok_openblock;
    }
    if (LastChar == '}') // ��� �ݱ�
    {
        LastChar = NextCh(Code, Idx);
        return tok_closeblock;
    }
    if (LastChar == '(') // ��ȣ ����
    {
        LastChar = NextCh(Code, Idx);
        return tok_openbrkt;
    }
    if (LastChar == ')') // ��ȣ �ݱ�
    {
        LastChar = NextCh(Code, Idx);
        return tok_closebrkt;
    }

    // ������ ���̸� EOF ��ū ��ȯ
    if (LastChar == EOF)
        return tok_eof;

    // �ƹ� ������ ������ ���� ��� �ƽ�Ű �ڵ� �� �״�� ��ȯ
    int ThisChar = LastChar;
    LastChar = NextCh(Code, Idx);
    return ThisChar;
}