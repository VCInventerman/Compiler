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

struct EmptyExpression : public Expression {};

struct Return : public Expression {
	Expression* ret = nullptr;

	void emitDependency(FuncEmitter& out) override {
		if (ret) {
			ret->emitDependency(out);
		}

		out.nextReg(); // Return consumes a register

		if (ret) {
			out << "\tret " << ret->getOperandType() << " " << ret->getOperand() << "\n";
		}
		else {
			out << "\tret void\n";
		}
	}
};

struct ArithmeticConversion : public Expression {
	int inRegister = 0;
	int outRegister = 0;

	void emitDependency(FuncEmitter& out) override {
		out << "%4 = fptosi float %3 to i32";
	}

	std::string getOperand() override { throw NULL; };

	std::string getOperandType() override { return "i" + std::to_string(currentDataModel->intWidth); }
};



struct Literal : public Expression {

};

struct IntegerLiteral : public Expression {
	int _val;

	int _valReg;

	IntegerLiteral(int val) : _val(val) {

	}

	void emitDependency(FuncEmitter& out) override {
		_valReg = out.nextReg();

		//todo: custom alignment for start and custom padding for end
		/*out << "\t%" << _ptrReg << " = alloca i32, align 4\n"
			<< "\tstore i32 " << _val << ", i32* %" << _ptrReg << ", align 4\n"
			<< "\t%" << _valReg << " = load i32, i32* %" << _ptrReg << ", align 4\n";*/
		out << "\t%" << _valReg << " = add nsw " << "i32" << " 0, " << _val << "\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	std::string getOperandType() override { 
		return buildStr("i", currentDataModel->intWidth * 8); 
	}
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

	std::string getOperand() override {
		buildStr("i8* getelementptr inbounds ([", llvmStr.size(), " x i8], [", llvmStr.size(), " x i8]* ", name, " , i64 0, i64 0)");
	}

	std::string getOperandType() override { return "i8*"; }
};

struct FunctionCall : public Expression {
	Function* _fn;

	int _retReg = 0;

	std::vector<Expression*> arguments;

	FunctionCall(Function* fn) : _fn(fn) {}

	void emitDependency(FuncEmitter& out) override;

	std::string getOperand() override;

	std::string getOperandType() override;
};

struct VariableDeclExp : public Expression {
	VariableDeclaration* decl;
	int valReg = 0;

	VariableDeclExp(VariableDeclaration* decl_) : decl(decl_) {}

	void emitDependency(FuncEmitter& out) override {
		decl->initializer->emitDependency(out);
		
		// Allocate a slot for the variable and assign %varname to its address
		out << "\t%" << decl->name << " = alloca " << decl->type->llvmName << "\n";

		// Put the output from the initializer in it
		out << "\tstore " << decl->type->llvmName << " " << decl->initializer->getOperand()
		    << ", " << decl->type->llvmName << "* %" << decl->name << ", align 4\n";

		// Assign %varname.0 to its current value
		out << "\t" << decl->getValReg() << " = load " << decl->type->llvmName << ", " 
			<< decl->type->llvmName << "* %" << decl->name << ", align 4\n";
	}

	// Only usable in if statements
	std::string getOperand() override {
		return decl->getValReg();
	}

	std::string getOperandType() override {
		return decl->type->llvmName;
	}
};

struct VariableRef : public Expression {
	VariableDeclaration* decl;

	VariableRef(VariableDeclaration* decl_) : decl(decl_) {}

	std::string getOperandType() override {
		return decl->type->llvmName;
	}

	std::string getOperand() override {
		return decl->getValReg();
	}

	void assign(FuncEmitter& out, std::string newValue) {
		auto reg = decl->getNextValReg();
		
		// Put the output in it
		out << "\tstore " << decl->type->llvmName << " " << newValue
			<< ", " << decl->type->llvmName << "* %" << decl->name << ", align 4\n";

		// Assign %varname.<num> to its current value
		out << "\t" << decl->getValReg() << " = load " << decl->type->llvmName << ", "
			<< decl->type->llvmName << "* %" << decl->name << ", align 4\n";
	}
};

struct BinaryOperator : public Expression {
	Expression* _lhs;
	Expression* _rhs;
	int _valReg;
	std::string op;

	BinaryOperator(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}

	void emitDependency(FuncEmitter& out) override {
		_lhs->emitDependency(out);
		_rhs->emitDependency(out);

		_valReg = out.nextReg();
		out << "\t%" << _valReg << " = " << op << " " << _lhs->getOperandType() << " "
			<< _lhs->getOperand() << ", " << _rhs->getOperand() << "\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
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

struct Assignment : public Expression {
	Expression* _lhs;
	Expression* _rhs;

	Assignment(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}

	void emitDependency(FuncEmitter& out) override {
		_lhs->emitDependency(out);
		_rhs->emitDependency(out);

		_lhs->assign(out, _rhs->getOperand());
	}

	std::string getOperand() override {
		return _lhs->getOperand();
	}

	std::string getOperandType() override {
		return _lhs->getOperandType();
	}
};

struct UnaryAdd : public Expression {
	Expression* _operand;

	UnaryAdd(Expression* operand) : _operand(operand) {}

	void emitDependency(FuncEmitter& out) override {
		// Identity operation
	}

	std::string getOperand() override {
		return _operand->getOperand();
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

	void emitDependency(FuncEmitter& out) override {
		_operand->emitDependency(out);

		_valReg = out.nextReg();
		out << "\t%" << _valReg << " = sub nsw " << _operand->getOperandType() << " 0, " << _operand->getOperand() << "\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
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
	if (type == "=") { return new Assignment(lhs, rhs); }

	return nullptr;
}

#endif // ifndef COMPILER_EXPRESSION_H