#ifndef COMPILER_TOKEN_H
#define COMPILER_TOKEN_H

#include <string_view>
#include <any>

#include "source.h"

enum class TokenType {
	UNKNOWN,
	END_OF_FIELD,
	PRIMITIVE,
	PLUS,
	MINUS,
	MULT,
	DIV,
	MODULO,
	BITSHIFT_LEFT,
	BITSHIFT_RIGHT,
	ASSIGNMENT,
	VARIABLE,
	SEMICOLON,
	FUNCTION,
	PRINT,
	LEFT_BRACE,
	RIGHT_BRACE,
	COLON,
	LEFT_PAREN,
	RIGHT_PAREN,
	LEFT_ANGLE,
	RIGHT_ANGLE,
};

constexpr int OPERATOR_PRECEDENCE[] = {
	999,
	999,
	999,
	6,
	6,
	5,
	5,
	5,
	7,
	7,
	10,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

constexpr std::string_view OPERATOR_TOKENS[] = {
	"",
	"",
	"",
	"+",
	"-",
	"*",
	"/",
	"%",
	"<<",
	">>",
	"=",
	"",
	";",
	"",
	"print",
	"{",
	"}",
	":",
	"(",
	")",
	"<",
	">",
};

constexpr char OPERATOR_CHARACTERS[] = {
	'+',
	'-',
	'*',
	'/',
	'%',
	'<',
	'>',
	'=',
	';',
	'&',
	'|',
	'^',
	'.',

	'{',
	'}',
	'[',
	']',
	'(',
	')',
	'\\',
	'\'',
	'\"'
};

class Token {
public:
	SourcePos code;

	TokenType type;
	std::any defaultValue;
};

enum class PrimaryValueCategory {
	PRVALUE, // Pure rvalue
	XVALUE, // Expiring value
	LVALUE // glvalue that is not an xvalue
};

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


struct Expression {
	virtual void emitFileScope(FuncEmitter& out) { throw NULL; }

	virtual void emitFunctionScope(FuncEmitter& out) { throw NULL; }

	virtual void emitOperand(FuncEmitter& out, bool includeType) { throw NULL; }
};

struct Conversion {

};

struct Return {
	Expression* ret;

	void emitFileScope(FuncEmitter& out) {}

	void emitFunctionScope(FuncEmitter& out) {
		ret->emitFunctionScope(out);

		out.nextReg(); // Return consumes a register

		if (ret) {
			out << "\tret ";
			ret->emitOperand(out, true);
			out << "\n";
		}
		else {
			out << "\tret void\n";
		}
	}

	void emitOperand(FuncEmitter& out, bool includeType) {
		throw NULL;
	}
};

struct FloatToInt {
	int inRegister = 0;
	int outRegister = 0;

	virtual void emitFunctionScope(FuncEmitter& out) {
		out << "%4 = fptosi float %3 to i32";
	}

	virtual void emitOperandScope(FuncEmitter& out);
};



struct Literal : public Expression {
	
};

struct IntegerLiteral : public Expression {
	int _val;

	int _ptrReg;
	int _valReg;

	IntegerLiteral(int val) : _val(val) {

	}

	void emitFunctionScope(FuncEmitter& out) {
		_ptrReg = out.nextReg();
		_valReg = out.nextReg();

		//todo: custom alignment for start and custom padding for end
		out << "\t%" << _ptrReg << " = alloca i32, align 4\n"
			<< "\tstore i32 " << _val << ", i32* %" << _ptrReg << ", align 4\n"
			<< "\t%" << _valReg << " = load i32, i32* %" << _ptrReg << ", align 4\n";
	}

	void emitOperand(FuncEmitter& out, bool includeType) {
		out << (includeType ? "i32 " : "") << "%" << _valReg;
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

	void emitFileScope(std::stringstream out) {
		static int index = 0;

		//todo: custom alignment for start and custom padding for end
		out << "@.str." << index << " = private unnamed_addr constant [" << (llvmStr.size() + 1) << " x i8] c\"" <<
			llvmStr << "\", align 1";
	}

	void emitOperand(std::stringstream out, bool includeType) {
		out << "i8* getelementptr inbounds ([" << llvmStr.size() << " x i8], [" << llvmStr.size() << " x i8]* " << name << " , i64 0, i64 0)";
	}
};

struct BinaryOperator : public Expression {
	Expression* _lhs;
	Expression* _rhs;
	int _valReg;
	std::string op;

	BinaryOperator(Expression* lhs, Expression* rhs) : _lhs(lhs), _rhs(rhs) {}

	void emitFunction(FuncEmitter& out) {
		_lhs->emitFunctionScope(out);
		_rhs->emitFunctionScope(out);

		_valReg = out.nextReg();
		out << "\t%" << _valReg << " = " << op << " ";
		_lhs->emitOperand(out, true);
		out << ", ";
		_rhs->emitOperand(out, false);
		out << "\n";
	}

	void emitOperand(FuncEmitter& out, bool includeType) {
		out << (includeType ? "i32 " : "") << "%" << _valReg;
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

/*


struct Operator {
	virtual std::string emit(LlvmValue left, LlvmValue right);
};

struct Addition {
	static constexpr const char symbol = '+';

	std::string emit(LlvmValue left, LlvmValue right) {

	}
}*/

#endif