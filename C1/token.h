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
	"print"
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
};

class Token {
public:
	SourcePos code;

	TokenType type;
	std::any defaultValue;
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