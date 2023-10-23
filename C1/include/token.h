#ifndef COMPILER_TOKEN_H
#define COMPILER_TOKEN_H

#include <string_view>
#include <any>
#include <vector>

#include "source.h"
#include "error.h"

enum class DataModels {
	// Bytes per int/long/pointer
	LP32, // 2/4/4
	ILP32, // 4/4/4

	LLP64, // 4/4/8
	LP64, // 4/8/8

	ILP64, // 8/8/8

	ARDUINO_OLD, // 2/4/4
	ARDUINO, // 4/8/4
};

struct DataModel {
	int charWidth;
	int shortWidth;
	int intWidth;
	int longWidth;
	int longLongWidth;
	int longDoubleWidth;

	int pointerWidth;
};

constexpr const DataModel DATA_MODELS[] = {
	{ 1, 2, 2, 4, 8, 10, 4 }, // LP32
	{ 1, 2, 4, 4, 8, 10, 4 }, // ILP32
	{ 1, 2, 4, 4, 8, 10, 8 }, // LLP64
	{ 1, 2, 4, 8, 8, 10, 8 }, // LP64

	{ 1, 4, 8, 8, 8, 10, 8 }, // ILP64

	{ 1, 2, 2, 4, 8, 10, 4 }, //todo: this may not be true
	{ 1, 2, 4, 8, 8, 10, 4 },
};

const DataModel* currentDataModel = &DATA_MODELS[(int)DataModels::LP64];


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
	AMPERSAND,
	IDENTIFIER, // std::string_view
	COMMA,
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
	3,
	999,
	999,
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
	"&",
	"",
	",",
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
	'\"',

	',',
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

#endif