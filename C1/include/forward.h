#ifndef COMPILER_FORWARD_H
#define COMPILER_FORWARD_H

#include <sstream>
#include <string>
#include <iostream>
#include <vector>

struct FuncEmitter {
	struct Deindenter {
		void operator()(FuncEmitter* out) {
			out->removeIndent();
		}
	};

	int regCnt = 0;
	int labelCnt = 1;
	int branchNum = 0;

	int indentLevel = 1;

	std::stringstream codeOut;

	std::unique_ptr<FuncEmitter, Deindenter> addIndent() {
		indentLevel++;
		return std::unique_ptr<FuncEmitter, Deindenter>(this, Deindenter());
	}
	void removeIndent() {
		indentLevel = indentLevel > 0 ? indentLevel - 1 : 0;
	}

	FuncEmitter& indent() {
		for (int i = 0; i < indentLevel; i++) {
			codeOut << '\t';
		}

		return *this;
	}

	template <typename T>
	FuncEmitter& operator<<(T elm) {
		codeOut << elm;
		return *this;
	}

	int nextReg() {
		return regCnt++;
	}

	std::vector<std::string> outputNames;

	void setNextOutputName(std::string name) {
		outputNames.push_back(name);
	}

	std::string getNextOutputName() {
		std::string ret = outputNames.back();
		outputNames.pop_back();
		return ret;
	}

	std::string nextBranchName() {
		return buildStr("b.", branchNum++);
	}
};

struct Function;
struct Expression;
struct CppType;

struct FunctionArgument {
	std::string_view name;
	CppType* type;
	Expression* value;
};

struct FunctionPrototype {
	std::string_view name;

	std::vector<FunctionArgument*> arguments;
	CppType* returnType;
};

struct Expression {
	// Do any work required to get this expression ready, such as allocating static memory
	virtual void emitFileScope(FuncEmitter& out) {}

	// Do work in function scope to get this expression ready, such as performing an operation on registers
	virtual void emitDependency(FuncEmitter& out) {}

	virtual CppType* getResultType() {
		std::cout << "Tried to use expression that doesn't yield a type!\n";
		throw NULL;
	}

	// Get the register this expression's value is in
	virtual std::string getOperand() {
		std::cout << "Tried to use expression that doesn't yield a value as a value!\n";
		throw NULL;
	}

	// Treating the expression as an lvalue, replace its current value
	// getOperand must now refer to the updated value taken from newValue
	virtual void assign(FuncEmitter& out, std::string newValue) {
		std::cout << "Tried to assign to a non-lvalue!\n";
		throw NULL;
	}
};

#endif // ifndef COMPILER_FORWARD_H