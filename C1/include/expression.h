#ifndef COMPILER_EXPRESSION_H
#define COMPILER_EXPRESSION_H

#include <sstream>
#include <iostream>

#include "forward.h"
#include "cppType.h"
#include "token.h"
#include "type.h"


struct Conversion : public Expression {

};

struct EmptyExpression : public Expression {
	void emitFileScope(FuncEmitter& out) override {}
	void emitFunctionScope(FuncEmitter& out) override {}
};

struct Return : public Expression {
	Expression* ret = nullptr;

	void emitFileScope(FuncEmitter& out) override {}

	void emitFunctionScope(FuncEmitter& out) override {
		if (ret) {
			ret->emitFunctionScope(out);
		}

		out.nextReg(); // Return consumes a register

		if (ret) {
			out << "\tret " << ret->getOperandType() << " ";
			ret->emitOperand(out);
			out << "\n";
		}
		else {
			out << "\tret void\n";
		}
	}
};

struct ArithmeticConversion : public Expression {
	int inRegister = 0;
	int outRegister = 0;

	virtual void emitFunctionScope(FuncEmitter& out) override {
		out << "%4 = fptosi float %3 to i32";
	}

	virtual void emitOperand(FuncEmitter& out) override {};

	std::string getOperandType() override { return "i" + std::to_string(currentDataModel->intWidth); }
};



struct Literal : public Expression {

};

struct IntegerLiteral : public Expression {
	int _val;

	int _ptrReg;
	int _valReg;

	IntegerLiteral(int val) : _val(val) {

	}

	void emitFunctionScope(FuncEmitter& out) override {
		_ptrReg = out.nextReg();
		_valReg = out.nextReg();

		//todo: custom alignment for start and custom padding for end
		out << "\t%" << _ptrReg << " = alloca i32, align 4\n"
			<< "\tstore i32 " << _val << ", i32* %" << _ptrReg << ", align 4\n"
			<< "\t%" << _valReg << " = load i32, i32* %" << _ptrReg << ", align 4\n";
	}

	void emitOperand(FuncEmitter& out) override {
		out << "%" << _valReg;
	}

	std::string getOperandType() override { return "i" + std::to_string(currentDataModel->intWidth * 8); }
};

struct StringLiteral : public Expression {
	std::string_view origStr;
	std::string rawStr;
	std::string llvmStr;

	std::string name;

	StringLiteral(std::string_view origStr_) : origStr(origStr_) {
		// Ensures every string literal has a unique name
		static int index = 0;

		name = "@.str." + std::to_string(index);

		rawStr = parseSourceStr(origStr);
		llvmStr = makeLlvmStr(rawStr);
	}

	static std::string parseSourceStr(std::string_view str) {
		constexpr std::pair<std::string_view, char> SIMPLE_ESCAPE_SEQUENCES[11] = {
			{ "\\\'", 0x27 },
			{ "\\\"", 0x22 },
			{ "\\?", 0x3f },
			{ "\\\\", 0x5c },
			{ "\\a", 0x07 },
			{ "\\b", 0x08 },
			{ "\\f", 0x0c },
			{ "\\n", 0x0a },
			{ "\\r", 0x0d },
			{ "\\t", 0x09 },
			{ "\\v", 0x0b },
		};

		std::string out;

		for (int i = 0; i < str.size(); i++) {
			int c = str[i];

			auto next = [&]() { return (i < str.size() - 1) ? (int)str[i] : EOF; };

			auto simpleReplacement = std::find_if(std::begin(SIMPLE_ESCAPE_SEQUENCES), std::end(SIMPLE_ESCAPE_SEQUENCES),
				[&](const std::pair<std::string_view, char> i) {
					return i.first[1] == next();
				});
			if (c == '\\' && simpleReplacement != std::end(SIMPLE_ESCAPE_SEQUENCES)) {
				out += simpleReplacement->second;

				i++;
			}
			else {
				out += c;
			}
		}

		out += '\0';
		return out;
	}

	static std::string makeLlvmStr(std::string_view str) {
		std::string out;

		for (int i = 0; i < str.size(); i++) {
			int c = str[i];

			out += "\\";

			out += std::to_string(c);
		}

		out += '\0';
		return out;
	}

	void emitFileScope(FuncEmitter& out) override {
		static int index = 0;

		//todo: custom alignment for start and custom padding for end
		out << "@.str." << index << " = private unnamed_addr constant [" << (llvmStr.size() + 1) << " x i8] c\"" <<
			llvmStr << "\", align 1";
	}

	void emitOperand(FuncEmitter& out) override {
		out << "i8* getelementptr inbounds ([" << llvmStr.size() << " x i8], [" << llvmStr.size() << " x i8]* " << name << " , i64 0, i64 0)";
	}

	std::string getOperandType() override { return "i8*"; }
};

struct FunctionCall : public Expression {
	Function* _fn;

	int _retReg = 0;

	std::vector<Expression*> arguments;

	FunctionCall(Function* fn) : _fn(fn) {}

	void emitFunctionScope(FuncEmitter& out) override;

	void emitOperand(FuncEmitter& out) override;

	std::string getOperandType() override;
};

struct VariableDeclExp : public Expression {
	VariableDeclaration* decl;

	VariableDeclExp(VariableDeclaration* decl_) : decl(decl_) {}

	void emitFunctionScope(FuncEmitter& out) override {
		decl->initializer->emitFunctionScope(out);
		emitOperand(out);
		out << " = ";
		decl->initializer->emitOperand(out);
		out << "\n";
	}

	// Only usable in if statements
	void emitOperand(FuncEmitter& out) override {
		out << "%" << decl->name << "." << decl->regNum;
	}

	std::string getOperandType() override {
		return decl->type->llvmName;
	}
};

struct VariableRef : public Expression {
	VariableDeclaration* decl;

	VariableRef(VariableDeclaration* decl_) : decl(decl_) {}

	void emitFunctionScope(FuncEmitter& out) override {}

	void emitOperand(FuncEmitter& out) override {
		out << "%" << decl->name << "." << decl->regNum;
	}

	void emitWriteSlot(FuncEmitter& out) override {
		out << "%" << decl->name << "." << ++decl->regNum;
	}

	std::string getOperandType() override {
		return decl->type->llvmName;
	}
};

struct BinaryOperator : public Expression {
	Expression* _lhs;
	Expression* _rhs;
	int _valReg;
	std::string op;

	BinaryOperator(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}

	void emitFunctionScope(FuncEmitter& out) override {
		_lhs->emitFunctionScope(out);
		_rhs->emitFunctionScope(out);

		_valReg = out.nextReg();
		out << "\t%" << _valReg << " = " << op << " " << _lhs->getOperandType() << " ";
		_lhs->emitOperand(out);
		out << ", ";
		_rhs->emitOperand(out);
		out << "\n";
	}

	void emitOperand(FuncEmitter& out) override {
		out << "%" << _valReg;
	}

	std::string getOperandType() override {
		return _lhs->getOperandType();
	}
};

struct Addition : public BinaryOperator {
	Addition(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "add nsw";
	}
};

struct Subtraction : public BinaryOperator {
	Subtraction(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "sub nsw";
	}
};

struct Multiplication : public BinaryOperator {
	Multiplication(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "mul nsw";
	}
};

struct Division : public BinaryOperator {
	Division(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "sdiv";
	}
};

struct Remainder : public BinaryOperator {
	Remainder(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "srem";
	}
};

struct BitAnd : public BinaryOperator {
	BitAnd(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "and";
	}
};

struct BitOr : public BinaryOperator {
	BitOr(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "or";
	}
};

struct BitXor : public BinaryOperator {
	BitXor(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "xor";
	}
};

struct BitShiftLeft : public BinaryOperator {
	BitShiftLeft(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "shl";
	}
};

struct BitShiftRight : public BinaryOperator {
	BitShiftRight(Expression* lhs, Expression* rhs) : BinaryOperator(lhs, rhs) {
		op = "ashr";
	}
};

struct UnaryAdd : public Expression {
	Expression* _operand;

	UnaryAdd(Expression* operand) : _operand(operand) {}

	void emitFunctionScope(FuncEmitter& out) override {
		// Identity operation
	}

	void emitOperand(FuncEmitter& out) override {
		_operand->emitOperand(out);
	}

	std::string getOperandType() override {
		// should strip lvalue I think
		return _operand->getOperandType();
	}
};

struct UnarySub : public Expression {
	Expression* _operand;
	int _valReg;

	UnarySub(Expression* operand) : _operand(operand) {}

	void emitFunctionScope(FuncEmitter& out) override {
		_operand->emitFunctionScope(out);

		_valReg = out.nextReg();
		out << "\t%" << _valReg << " = sub nsw " << _operand->getOperandType() << " 0, ";
		_operand->emitOperand(out);
		out << "\n";
	}

	void emitOperand(FuncEmitter& out) override {
		out << "%" << _valReg;
	}

	std::string getOperandType() override {
		return _operand->getOperandType();
	}
};

inline Expression* makeBinaryExp(std::string_view type, Expression* lhs, Expression* rhs) {
	if (type == "+") { return new Addition(lhs, rhs); }
	if (type == "-") { return new Subtraction(lhs, rhs); }
	if (type == "*") { return new Multiplication(lhs, rhs); }
	if (type == "/") { return new Division(lhs, rhs); }

	return nullptr;
}

#endif // ifndef COMPILER_EXPRESSION_H