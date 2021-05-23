
// MicroSEL
// value.h

#pragma once

#include <cmath>

typedef enum ValueType
{
	val_data = 0,
	val_return = 1,
	val_break = 2,
	val_err = 3,
} vType;

class Value
{
	vType vt = val_data;
	double num;
public:
	Value() {}
	Value(vType vt, double dVal) : vt(vt), num(dVal) {}
	Value(double dVal) : num(dVal) {}

	void updateVal(double dVal) { num = dVal; }

	bool isErr() { return vt == val_err; }
	bool isInt() { return trunc(num) == num; }
	bool isUInt() { return isInt() && num >= 0; }

	vType getType() { return vt; }
	double getNum() { return num; }
};