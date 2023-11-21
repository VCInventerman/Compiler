#ifndef COMPILER_TOKEN_H
#define COMPILER_TOKEN_H

#include <string_view>
#include <vector>
#include <variant>

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

enum class Platform {
	WINDOWS,
	LINUX
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

constexpr const inline DataModel DATA_MODELS[] = {
	{ 8, 16, 16, 32, 64, 80, 32 }, // LP32
	{ 8, 16, 32, 32, 64, 80, 32 }, // ILP32
	{ 8, 16, 32, 32, 64, 80, 64 }, // LLP64
	{ 8, 16, 32, 64, 64, 80, 64 }, // LP64

	{ 8, 32, 64, 64, 64, 80, 64 }, // ILP64

	{ 8, 16, 16, 32, 64, 80, 32 }, //todo: this may not be true
	{ 8, 16, 32, 64, 64, 80, 32 },
};

const static DataModel* currentDataModel = &DATA_MODELS[(int)DataModels::LP64];
static Platform targetPlatform = Platform::WINDOWS;


enum class TokenType {
	UNKNOWN,
	END_OF_FIELD,

	INTEGER_LITERAL,
	BOOL_LITERAL, // 0 or 1
	FLOAT_LITERAL, // double
	STRING_LITERAL, // An array of values

	IDENTIFIER,
	Operator,
};

enum class Operator {
	// 1 Left-to-right
	SCOPE_RESOLUTION,

	// 2 Left-to-right
	POSTFIX_INCREMENT, 
	POSTFIX_DECREMENT,
	FUNCTIONAL_CAST,
	FUNCTION_CALL,
	SUBSCRIPT,
	MEMBER_ACCESS_DOT,
	MEMBER_ACCESS_ARROW,

	// 3 Right-to-left
	PREFIX_INCREMENT,
	PREFIX_DECREMENT,
	LOGICAL_NOT,
	BITWISE_NOT,
	C_CAST,
	DEREFERENCE,
	ADDRESS_OF,
	SIZEOF,
	CO_AWAIT,
	NEW,
	NEW_ARRAY,
	DELETE,
	DELETE_ARRAY,

	// 4 Left-to-right
	POINTER_TO_MEMBER_DOT,
	POINTER_TO_MEMBER_ARROW,

	// 5 Left-to-right
	MULTIPLICATION,
	DIVISION,
	REMAINDER,

	// 6 Left-to-right
	ADDITION,
	SUBTRACTION,

	// 7 Left-to-right
	BITWISE_LEFT_SHIFT,
	BITWISE_RIGHT_SHIFT,

	// 8 Left-to-right
	THREE_WAY_COMPARISON,

	// 9 Left-to-right
	LESSER_THAN,
	LESSER_THAN_OR_EQUAL,
	GREATER_THAN,
	GREATER_THAN_OR_EQUAL,

	// 10 Left-to-right
	EQUAL,
	NOT_EQUAL,

	// 11 Left-to-right
	BITWISE_AND,

	// 12 Left-to-right
	BITWISE_XOR,

	// 13 Left-to-right
	BITWISE_OR,
	
	// 14 Left-to-right
	LOGICAL_AND,
	
	// 15 Left-to-right
	LOGICAL_OR,
	
	// 16 Right-to-left
	TERNARY_CONDITIONAL,
	THROW,
	CO_YIELD,
	DIRECT_ASSIGNMENT,

	COMPOUND_ASSIGNMENT_ADDITION,
	COMPOUND_ASSIGNMENT_DIFFERENCE,
	COMPOUND_ASSIGNMENT_PRODUCT,
	COMPOUND_ASSIGNMENT_QUOTIENT,
	COMPOUND_ASSIGNMENT_REMAINDER,
	COMPOUND_ASSIGNMENT_BITWISE_LEFT_SHIFT,
	COMPOUND_ASSIGNMENT_BITWISE_RIGHT_SHIFT,
	COMPOUND_ASSIGNMENT_BITWISE_AND,
	COMPOUND_ASSIGNMENT_BITWISE_XOR,
	COMPOUND_ASSIGNMENT_BITWISE_OR,
	 
	// 17 Left-to-right
	// COMMA // No longer an operator, thank goodness

	UNKNOWN,
};

enum class OperatorBindingDirection {
	LEFT, // to right
	RIGHT // to left
};

struct OperatorTrait {
	Operator op;
	std::string_view str;
	int precedence;
	OperatorBindingDirection direction;
};

constexpr OperatorTrait OPERATOR_TRAITS[] = {
	{ Operator::SCOPE_RESOLUTION, "::", 1, OperatorBindingDirection::LEFT },

	{ Operator::POSTFIX_INCREMENT, "++", 2, OperatorBindingDirection::LEFT },
	{ Operator::POSTFIX_DECREMENT, "--", 2, OperatorBindingDirection::LEFT },
	{ Operator::FUNCTIONAL_CAST, " ", 2, OperatorBindingDirection::LEFT },
	{ Operator::FUNCTION_CALL, " ", 2, OperatorBindingDirection::LEFT },
	{ Operator::SUBSCRIPT, "[", 2, OperatorBindingDirection::LEFT },
	{ Operator::MEMBER_ACCESS_DOT, ".", 2, OperatorBindingDirection::LEFT },
	{ Operator::MEMBER_ACCESS_ARROW, "->", 2, OperatorBindingDirection::LEFT },

	{ Operator::PREFIX_INCREMENT, "++", 3, OperatorBindingDirection::RIGHT },
	{ Operator::PREFIX_DECREMENT, "--", 3, OperatorBindingDirection::RIGHT },
	{ Operator::LOGICAL_NOT, "!", 3, OperatorBindingDirection::RIGHT },
	{ Operator::BITWISE_NOT, "~", 3, OperatorBindingDirection::RIGHT },
	{ Operator::C_CAST, " ", 3, OperatorBindingDirection::RIGHT },
	{ Operator::DEREFERENCE, "*", 3, OperatorBindingDirection::RIGHT },
	{ Operator::ADDRESS_OF, "&", 3, OperatorBindingDirection::RIGHT },
	{ Operator::SIZEOF, " ", 3, OperatorBindingDirection::RIGHT },
	{ Operator::CO_AWAIT, " ", 3, OperatorBindingDirection::RIGHT },
	{ Operator::NEW, " ", 3, OperatorBindingDirection::RIGHT },
	{ Operator::NEW_ARRAY, " ", 3, OperatorBindingDirection::RIGHT },
	{ Operator::DELETE, " ", 3, OperatorBindingDirection::RIGHT },
	{ Operator::DELETE_ARRAY, " ", 3, OperatorBindingDirection::RIGHT },

	{ Operator::POINTER_TO_MEMBER_DOT, ".*", 4, OperatorBindingDirection::LEFT },
	{ Operator::POINTER_TO_MEMBER_ARROW, ".*", 4, OperatorBindingDirection::LEFT },

	{ Operator::MULTIPLICATION, "*", 5, OperatorBindingDirection::LEFT },
	{ Operator::DIVISION, "/", 5, OperatorBindingDirection::LEFT },
	{ Operator::REMAINDER, "%", 5, OperatorBindingDirection::LEFT },

	{ Operator::ADDITION, "+", 6, OperatorBindingDirection::LEFT },
	{ Operator::SUBTRACTION, "-", 6, OperatorBindingDirection::LEFT },

	{ Operator::BITWISE_LEFT_SHIFT, "<<", 7, OperatorBindingDirection::LEFT },
	{ Operator::BITWISE_RIGHT_SHIFT, ">>", 7, OperatorBindingDirection::LEFT },

	{ Operator::THREE_WAY_COMPARISON, "<=>", 8, OperatorBindingDirection::LEFT },

	{ Operator::LESSER_THAN, "<", 9, OperatorBindingDirection::LEFT },
	{ Operator::LESSER_THAN_OR_EQUAL, "<=", 9, OperatorBindingDirection::LEFT },
	{ Operator::GREATER_THAN, ">", 9, OperatorBindingDirection::LEFT },
	{ Operator::GREATER_THAN_OR_EQUAL, ">=", 9, OperatorBindingDirection::LEFT },

	{ Operator::EQUAL, "==", 10, OperatorBindingDirection::LEFT },
	{ Operator::NOT_EQUAL, "!=", 10, OperatorBindingDirection::LEFT },

	{ Operator::BITWISE_AND, "&", 11, OperatorBindingDirection::LEFT },

	{ Operator::BITWISE_XOR, "^", 12, OperatorBindingDirection::LEFT },

	{ Operator::BITWISE_OR, "|", 13, OperatorBindingDirection::LEFT },

	{ Operator::LOGICAL_AND, "&&", 14, OperatorBindingDirection::LEFT },

	{ Operator::LOGICAL_OR, "||", 15, OperatorBindingDirection::LEFT },

	{ Operator::TERNARY_CONDITIONAL, "?", 16, OperatorBindingDirection::LEFT },
	{ Operator::THROW, " ", 16, OperatorBindingDirection::LEFT },
	{ Operator::CO_YIELD, " ", 16, OperatorBindingDirection::LEFT },
	{ Operator::DIRECT_ASSIGNMENT, "=", 16, OperatorBindingDirection::LEFT },

	{ Operator::COMPOUND_ASSIGNMENT_ADDITION, "+=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_DIFFERENCE, "-=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_PRODUCT, "*=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_QUOTIENT, "/=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_REMAINDER, "%=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_BITWISE_LEFT_SHIFT, "<<=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_BITWISE_RIGHT_SHIFT, ">>=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_BITWISE_AND, "&=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_BITWISE_XOR, "^=", 16, OperatorBindingDirection::RIGHT },
	{ Operator::COMPOUND_ASSIGNMENT_BITWISE_OR, "|=", 16, OperatorBindingDirection::RIGHT },

	{ Operator::UNKNOWN, "(", 999 },
	{ Operator::UNKNOWN, ")", 999 },
	{ Operator::UNKNOWN, "{", 999 },
	{ Operator::UNKNOWN, "}", 999 },
	{ Operator::UNKNOWN, "[", 999 },
	{ Operator::UNKNOWN, "]", 999 },
	{ Operator::UNKNOWN, ";", 999 },
	{ Operator::UNKNOWN, ",", 999 },
};

using LiteralContainerEmpty = std::monostate;
using LiteralContainer = std::variant<LiteralContainerEmpty, int64_t, uint64_t, double, std::string>;

class Token {
public:
	SourcePos origCode;

	TokenType type = TokenType::UNKNOWN;
	std::string_view str;
	LiteralContainer value;
	Operator op;
};

#endif