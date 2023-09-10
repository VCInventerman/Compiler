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
	ASSIGNMENT
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
};

struct Token {
	SourcePos code;

	TokenType type;
	std::any defaultValue;
};

#endif