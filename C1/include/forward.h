#ifndef COMPILER_FORWARD_H
#define COMPILER_FORWARD_H

#include <sstream>
#include <string>
#include <iostream>
#include <vector>

struct FuncEmitter {
	int regCnt = 1;
	int labelCnt = 1;

	std::stringstream codeOut;

	template <typename T>
	FuncEmitter& operator<<(T elm) {
		codeOut << elm;
		return *this;
	}

	int nextReg() {
		return regCnt++;
	}
};

struct Function;
struct Expression;
struct CppType;

struct FunctionArgument {
	std::string_view name;
	CppType* type;
	Expression* defaultValue;
};

struct FunctionPrototype {
	std::string_view name;

	std::vector<FunctionArgument> arguments;
	CppType* returnType;
};

struct Expression {
	virtual void emitFileScope(FuncEmitter& out) { throw NULL; }

	virtual void emitFunctionScope(FuncEmitter& out) { throw NULL; }

	virtual void emitOperand(FuncEmitter& out) { throw NULL; }

	virtual std::string getOperandType() {
		std::cout << "Tried to use expression that doesn't yield a type!\n";
		throw NULL;
	}
};

#endif // ifndef COMPILER_FORWARD_H