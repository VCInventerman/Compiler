#ifndef COMPILER_EXPRESSION_H
#define COMPILER_EXPRESSION_H

#include <sstream>
#include <iostream>
#include <optional>

#include "forward.h"
#include "cppType.h"
#include "token.h"
#include "type.h"
#include "util.h"


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
			out.indent() << "ret " << ret->getResultType()->getLlvmName() << " " << ret->getOperand() << "\n";
		}
		else {
			out.indent() << "ret void\n";
		}
	}
};

struct ArithmeticConversion : public Expression {
	int inRegister = 0;
	int outRegister = 0;

	void emitDependency(FuncEmitter& out) override {
		out.indent() << "%4 = fptosi float %3 to i32";
	}

	std::string getOperand() override { throw NULL; };

	//CppType* getResultType() override { return "i" + std::to_string(currentDataModel->intWidth); }
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

		out.indent() << "%" << _valReg << " = add nsw " << "i32" << " 0, " << _val << "\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	CppType* getResultType() override { 
		return strToType("int"); 
	}
};

struct BoolLiteral : public Expression {
	bool _val;

	BoolLiteral(int val) : _val(val == 0) {}

	std::string getOperand() override {
		return _val ? "true" : "false";
	}

	CppType* getResultType() override {
		return strToType("bool");
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
		out.indent() << "@.str." << index << " = private unnamed_addr constant [" << (llvmStr.size() + 1) << " x i8] c\"" <<
			llvmStr << "\", align 1";
	}

	std::string getOperand() override {
		return buildStr("i8* getelementptr inbounds ([", llvmStr.size(), " x i8], [", llvmStr.size(), " x i8]* ", name, " , i64 0, i64 0)");
	}

	CppType* getResultType() override { return strToType("char*"); }
};

struct FunctionCall : public Expression {
	Function* _fn;

	int _retReg = 0;

	std::vector<Expression*> arguments;

	FunctionCall(Function* fn) : _fn(fn) {}

	void emitDependency(FuncEmitter& out) override;

	std::string getOperand() override;

	CppType* getResultType() override;
};

// Every function argument gets converted into an actual variable so that it can be written to
// This provdides the initial reference to the argument's value
struct FunctionArgumentInitializer : public Expression {
	FunctionArgument* _arg;

	FunctionArgumentInitializer(FunctionArgument* arg) : _arg(arg) {}

	std::string getOperand() override {
		return buildStr("%", _arg->name, ".arg");
	}

	CppType* getResultType() override {
		return _arg->type;
	}
};

struct VariableDeclExp : public Expression {
	VariableDeclaration* decl;
	int valReg = 0;

	VariableDeclExp(VariableDeclaration* decl_) : decl(decl_) {}

	void emitDependency(FuncEmitter& out) override {
		decl->initializer->emitDependency(out);
		
		// Allocate a slot for the variable and assign %varname to its address
		out.indent() << "%" << decl->name << " = alloca " << decl->type->getLlvmName() << "\n";

		// Put the output from the initializer in it
		out.indent() << "store " << decl->type->getLlvmName() << " " << decl->initializer->getOperand()
		    << ", " << decl->type->getLlvmName() << "* %" << decl->name << ", align 4\n";

		// Assign %varname.0 to its current value
		out.indent() << decl->getValReg() << " = load " << decl->type->getLlvmName() << ", " 
			<< decl->type->getLlvmName() << "* %" << decl->name << ", align 4\n";
	}

	// Only usable in if statements
	std::string getOperand() override {
		return decl->getValReg();
	}

	CppType* getResultType() override {
		return decl->type;
	}
};

struct VariableRef : public Expression {
	VariableDeclaration* decl;

	VariableRef(VariableDeclaration* decl_) : decl(decl_) {}

	CppType* getResultType() override {
		return decl->type;
	}

	void emitDependency(FuncEmitter& out) override {
		// Assign %varname.<num> to its current value
		auto reg = decl->getNextValReg();

		out.indent() << decl->getValReg() << " = load " << decl->type->getLlvmName() << ", "
			<< decl->type->getLlvmName() << "* %" << decl->name << ", align 4\n";
	}

	std::string getOperand() override {
		return decl->getValReg();
	}

	void assign(FuncEmitter& out, std::string newValue) {
		// Put the output in it
		out.indent() << "store " << decl->type->getLlvmName() << " " << newValue
			<< ", " << decl->type->getLlvmName() << "* %" << decl->name << ", align 4\n";
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
		out.indent() << "%" << _valReg << " = " << op << " " << _lhs->getResultType()->getLlvmName() << " "
			<< _lhs->getOperand() << ", " << _rhs->getOperand() << "\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	CppType* getResultType() override {
		return _lhs->getResultType();
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

	CppType* getResultType() override {
		return _lhs->getResultType();
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

	CppType* getResultType() override {
		// should strip lvalue I think
		return _operand->getResultType();
	}
};

struct UnarySub : public Expression {
	Expression* _operand;
	int _valReg;

	UnarySub(Expression* operand) : _operand(operand) {}

	void emitDependency(FuncEmitter& out) override {
		_operand->emitDependency(out);

		_valReg = out.nextReg();
		out.indent() << "%" << _valReg << " = sub nsw " << _operand->getResultType()->getLlvmName() << " 0, " << _operand->getOperand() << "\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	CppType* getResultType() override {
		return _operand->getResultType();
	}
};

struct BitwiseNot : public Expression {
	Expression* _operand;
	int _valReg;

	BitwiseNot(Expression* operand) : _operand(operand) {}

	void emitDependency(FuncEmitter& out) override {
		_operand->emitDependency(out);

		_valReg = out.nextReg();
		out.indent() << "%" << _valReg << " = xor " << _operand->getResultType()->getLlvmName() << _operand->getOperand() << ", -1\n";
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	CppType* getResultType() override {
		return _operand->getResultType();
	}
};

struct Dereference : public Expression {
	Expression* _addr;
	CppType* _outType;
	int _valReg = -1;

	Dereference(Expression* addr_) : _addr(addr_) {
		_outType = new CppType(*addr_->getResultType());
		_outType->_pointerLayers -= 2;
		_outType->make();
	}

	void emitDependency(FuncEmitter& out) override {
		_addr->emitDependency(out);

		_valReg = out.nextReg();

		out.indent() << "%" << _valReg << " = load " << _outType->getLlvmName() << ", " << _addr->getResultType()->getLlvmName() << " " << _addr->getOperand() << '\n';
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	CppType* getResultType() override {
		return _outType;
	}

	void assign(FuncEmitter& out, std::string newValue) {
		out.indent() << "store " << _outType->getLlvmName() << " " << newValue
			<< ", " << _addr->getResultType()->getLlvmName() << " " << _addr->getOperand() << "\n";
	}
};

struct LogicEqual : public Expression {
	Expression* _lhs;
	Expression* _rhs;
	int _valReg = -1;

	LogicEqual(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}

	void emitDependency(FuncEmitter& out) override {
		_lhs->emitDependency(out);
		_rhs->emitDependency(out);

		int iValReg = out.nextReg();
		_valReg = out.nextReg();
		out.indent() << "%" << iValReg << " = icmp eq " << _lhs->getResultType()->getLlvmName() << " "
			<< _lhs->getOperand() << ", " << _rhs->getOperand() << "\n";

		out.indent() << "\t%" << _valReg << " = zext i1 %" << iValReg << " to i" << strToType("bool")->width() << '\n';
	}

	std::string getOperand() override {
		return buildStr("%", _valReg);
	}

	CppType* getResultType() override {
		return strToType("bool");
	}
};

struct LogicNotEqual : public Expression {
	Expression* _lhs;
	Expression* _rhs;
	int _valReg, _wideValReg;

	LogicNotEqual(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}

	void emitDependency(FuncEmitter& out) override {
		_lhs->emitDependency(out);
		_rhs->emitDependency(out);

		_valReg = out.nextReg();
		out.indent() << "%" << _valReg << " = icmp ne " << _lhs->getResultType()->getLlvmName() << " "
			<< _lhs->getOperand() << ", " << _rhs->getOperand() << "\n";

		_wideValReg = out.nextReg();
		out.indent() << "\t%" << _valReg << " = zext i1 %" << _wideValReg << " to i" << strToType("bool")->width() << '\n';
	}

	std::string getOperand() override {
		return buildStr("%", _wideValReg);
	}

	CppType* getResultType() override {
		return strToType("bool");
	}
};

struct Compare : public Expression {
	Expression* _lhs;
	Expression* _rhs;
	int _valReg, _wideValReg;
	std::string_view _op;

	Compare(Expression* lhs, Expression* rhs, std::string_view op) : _lhs(lhs), _rhs(rhs), _op(op) {}

	void emitDependency(FuncEmitter& out) override {
		_lhs->emitDependency(out);
		_rhs->emitDependency(out);

		_valReg = out.nextReg();
		out.indent() << "%" << _valReg << " = icmp " << _op << " " << _lhs->getResultType()->getLlvmName() << " "
			<< _lhs->getOperand() << ", " << _rhs->getOperand() << "\n";

		_wideValReg = out.nextReg();
		out.indent() << "\t%" << _wideValReg << " = zext i1 %" << _valReg << " to i" << strToType("bool")->width() << '\n';
	}

	std::string getOperand() override {
		return buildStr("%", _wideValReg);
	}

	CppType* getResultType() override {
		return strToType("bool");
	}
};

struct Cast : public Expression {
	Expression* _in;
	CppType* _outType;

	std::string _outReg;

	Cast(Expression* in, CppType* outType) : _in(in), _outType(outType) {}

	void emitDependency(FuncEmitter& out) override {
		_in->emitDependency(out);

		// Widening
		if (_in->getResultType()->width() < _outType->width() && _in->getResultType()->isInteger() && _outType->isInteger()) {
			int valReg = out.nextReg();
			_outReg = buildStr("%", valReg);

			out.indent() << _outReg << " = ";

			if (!_in->getResultType()->isSigned() && !_outType->isSigned()) {
				// Extend without sign
				out << "zext ";
			}
			else if (!_in->getResultType()->isSigned() && _outType->isSigned()) {
				// Extend and add sign
				out << "zext ";
			}
			else if (_in->getResultType()->isSigned() && _outType->isSigned()) {
				// Extend and keep sign
				out << "sext ";
			}
			else {
				throw NULL;
			}

			out.indent() << _in->getResultType()->getLlvmName() << " " << _in->getOperand()
				<< " to " << _outType->getLlvmName() << "\n";
		}
		else if (_in->getResultType()->_pointerLayers && _outType->_pointerLayers) {
			// Pointer-to-pointer cast
			_outReg = out.nextReg();

			out.indent() << "%" << _outReg << " = bitcast " << _in->getResultType()->getLlvmName() << " " <<
				_in->getOperand() << " to " << _outType->getLlvmName();
		}
		else if (_in->getResultType()->width() == _outType->width()) {
			_outReg = _in->getOperand();
			return;
		}
		else {
			// Truncate
			int valReg = out.nextReg();
			_outReg = buildStr("%", valReg);

			out.indent() << _outReg << " = trunc " << _in->getResultType()->getLlvmName() << 
				" " << _in->getOperand() << " to " << _outType->getLlvmName() << '\n';
		}
	}


	std::string getOperand() override {
		return _outReg;
	}

	CppType* getResultType() override {
		return _outType;
	}
};

inline Expression* makeBinaryExp(std::string_view type, Expression* lhs, Expression* rhs) {
	if (type == "+") { return new Addition(lhs, rhs); }
	if (type == "-") { return new Subtraction(lhs, rhs); }
	if (type == "*") { return new Multiplication(lhs, rhs); }
	if (type == "/") { return new Division(lhs, rhs); }
	if (type == "%") { return new Remainder(lhs, rhs); }
	if (type == "=") { return new Assignment(lhs, rhs); }

	if (type == "<<") { return new BitShiftLeft(lhs, rhs); }
	if (type == ">>") { return new BitShiftRight(lhs, rhs); }

	if (type == "<") { return new Compare(lhs, rhs, "slt"); }
	if (type == "<=") { return new Compare(lhs, rhs, "sle"); }
	if (type == ">") { return new Compare(lhs, rhs, "sgt"); }
	if (type == ">=") { return new Compare(lhs, rhs, "sge"); }

	if (type == "==") { return new LogicEqual(lhs, rhs); }
	if (type == "!=") { return new LogicNotEqual(lhs, rhs); }

	if (type == "&") { return new BitAnd(lhs, rhs); }
	if (type == "^") { return new BitXor(lhs, rhs); }
	if (type == "|") { return new BitOr(lhs, rhs); }

	// SHORT CIRCUIT
	//if (type == "&&") { return new Assignment(lhs, rhs); }
	//if (type == "||") { return new Assignment(lhs, rhs); }

	//if (type == "+=") { return new Assignment(lhs, rhs); }
	//if (type == "-=") { return new Assignment(lhs, rhs); }
	//if (type == "*=") { return new Assignment(lhs, rhs); }
	//if (type == "/=") { return new Assignment(lhs, rhs); }
	//if (type == "%=") { return new Assignment(lhs, rhs); }
	//if (type == "<<=") { return new Assignment(lhs, rhs); }
	//if (type == ">>=") { return new Assignment(lhs, rhs); }
	//if (type == "&=") { return new Assignment(lhs, rhs); }
	//if (type == "^=") { return new Assignment(lhs, rhs); }
	//if (type == "|=") { return new Assignment(lhs, rhs); }

	return nullptr;
}

#endif // ifndef COMPILER_EXPRESSION_H