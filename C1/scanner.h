#ifndef COMPILER_SCANNER_H
#define COMPILER_SCANNER_H

#include <string_view>
#include <array>
#include <functional>
#include <algorithm>

#include "error.h"
#include "util.h"
#include "token.h"

enum class SCAN_TOKEN_TYPES {
	END_OF_FIELD,
	IDENTIFIER,
	OPERATOR,
	SEMICOLON,

	KEYWORD
};

enum class KEYWORDS {
	ALIGNAS, // alignas()
	ALIGNOF, // alignof()
	AND, // &&
	AND_EQ, // &=
	ASM, // asm()
	AUTO, // auto
	BITAND, // &
	BITOR, // |
	BOOL, // bool
	BREAK, // break
	
	CASE, // case ():
	CATCH, // catch() {}
	CHAR, // char
	CHAR8_T, // char8_t
	CHAR16_T, // char16_t
	CHAR32_T, // char32_t
	CLASS, // class
	COMPL, // ~
	CONCEPT, // concept
	CONST, // const
	
	CONSTEVAL,
	CONSTEXPR,
	CONSTINIT,
	CONST_CAST,
	CONTINUE,
	CO_AWAIT,
	CO_RETURN,
	CO_YIELD,
	DECLTYPE,
	DEFAULT,
	
	DELETE,
	DO,
	DOUBLE,
	DYNAMIC_CAST,
	ELSE,
	ENUM,
	EXPLICIT,
	EXPORT,
	EXTERN,
	FALSE,
	
	FLOAT,
	FOR,
	FRIEND,
	GOTO,
	IF,
	INLINE,
	INT,
	LONG,
	MUTABLE,
	NAMESPACE,
	
	NEW,
	NOEXCEPT,
	NOT,
	NOT_EQ,
	NULLPTR,
	OPERATOR,
	OR,
	OR_EQ,
	PRIVATE,
	PROTECTED,
	
	PUBLIC,
	REGISTER,
	REINTERPRET_CAST,
	REQUIRES,
	RETURN,
	SHORT,
	SIGNED,
	SIZEOF,
	STATIC,
	STATIC_ASSERT,
	
	STATIC_CAST,
	STRUCT,
	SWITCH,
	TEMPLATE,
	THIS,
	THREAD_LOCAL,
	THROW,
	TRUE,
	TRY,
	TYPEDEF,
	
	TYPEID,
	TYPENAME,
	UNION,
	UNISGNED,
	USING,
	VIRTUAL,
	VOID,
	VOLATILE,
	WHCAR_T,
	WHILE,
	
	XOR,
	XOR_EQ
};
constexpr std::string_view KEYWORDS_STR[] = {
	"",
	"",
	"and",
	"and_eq",
	"asm",
	"auto",
	"bitand",
	"bitor",
	"bool",
	"break",

	"case",
	"catch",
	"char",
	"char8_t",
	"char16_t",
	"char32_t",
	"class",
	"",
	"",
	"const",

	"consteval",
	"constexpr",
	"constinit",
	"const_cast",
	"continue",
	"",
	"",
	"",
	"",
	"default",

	"delete",
	"do",
	"double",
	"dynamic_cast",
	"else",
	"enum",
	"explicit",
	"",
	"extern",
	"false",

	"float",
	"for",
	"friend",
	"goto",
	"if",
	"inline",
	"int",
	"long",
	"mutable",
	"namespace",

	"new",
	"noexcept",
	"not",
	"not_eq",
	"nullptr",
	"operator",
	"or",
	"or_eq",
	"private",
	"protected",

	"public",
	"register",
	"reinterpret_cast",
	"",
	"return",
	"short",
	"signed",
	"sizeof",
	"static",
	"static_assert",

	"static_cast",
	"struct",
	"switch",
	"template",
	"this",
	"thread_local",
	"throw",
	"true",
	"try",
	"typedef",

	"typeid",
	"typename",
	"union",
	"unsigned",
	"using",
	"virtual",
	"void",
	"volatile",
	"wchar_t",
	"while",

	"xor",
	"xor_eq"
};

enum class SPECIAL_IDENTIFIERS {
	FINAL,
	OVERRIDE,
	IMPORT,
	MODULE
};
constexpr std::string_view SPECIAL_IDENTIFIERS_STR[] = {
	"final",
	"override",
	"import",
	"module"
};

constexpr std::pair<std::string_view, std::string_view> KEYWORD_OPERATOR_ALIASES[] = {
	{ "and", "&& " },
	{ "and_eq", "&=    " },
	{ "bitand", "&     " },
	{ "bitor", "|    " },
	{ "compl", "~    " },
	{ "not", "!  " },
	{ "not_eq", "!=    " },
	{ "or", "||" },
	{ "or_eq", "|=   " },
	{ "xor", "^  " },
	{ "xor_eq", "^=    " },

	// No, I will not be including digraphs
};

struct Scanner {
	static constexpr const std::array<uint32_t, 5> WHITESPACE = { ' ', '\n', '\t', '\r', '\0' };

	using ItrT = const char*;


	// Begin and end sentinels
	std::string_view code;

	ItrT readCursor;

	// String identifying where the sequence being scanned came from
	std::string_view source;


	Scanner(std::string_view code_, std::string_view source_) : code(code_), source(source_) {
		readCursor = &*code.begin();
	}

	static ItrT skipws(ItrT readCursor, ItrT end) {
		while (readCursor != end && isAnyOf(*readCursor, WHITESPACE)) {
			readCursor++;
		}

		return readCursor;
	}

	SourcePos genSourcePos(ItrT start, ItrT end) {
		return { source, code, start, end };
	}

	// Consume a token, without modifying r
	// Returns: the token and the state of the read cursor if the token is kept
	std::pair<std::string_view, ItrT> peekAt(ItrT readCursor) {
		std::string_view tok = "";

		ItrT r = skipws(readCursor, code.data() + code.size());

		if (r == code.data() + code.size()) {
			return { "", r };
		}

		// Function evaluating whether a character matches the pattern started by its first
		std::function<bool(uint32_t)> check;

		uint32_t firstChar = *r;
		tok = std::string_view(&*r, 1);
		r++;
		if (r == code.data() + code.size()) { return { tok, r }; }

		if (isAnyOf(firstChar, OPERATOR_CHARACTERS)) {
			check = [&tok](uint32_t c) { 
				// Consume until there is not at least one operator with the same beginning characters
				bool matches = std::any_of(std::begin(OPERATOR_TOKENS), std::end(OPERATOR_TOKENS), [&tok](std::string_view test) { 
						return test.starts_with(tok);
					});

				if (matches) {
					return true;
				}
				else {
					tok.remove_suffix(1);
					return false;
				}
			};
		}
		// An identifier may start with an alphabetical character or an underscore. This will also match keywords.
		else if ((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z') || (firstChar == '_')) {
			check = [](uint32_t c) {
				// After the first character, numbers are allowed too
				return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
			};
		}
		// A literal may start with a number or a quotation mark
		else if (firstChar >= '0' && firstChar <= '9') {
			check = [](uint32_t c) {
				return c >= '0' && c <= '9';
			};
		}
		else {
			throw SourceError("Unexpected token beginning", genSourcePos(r, r));
		}

		while (check(*r)) {
			r++;
			if (r == code.data() + code.size()) { return { tok, r }; }
			tok = std::string_view(tok.data(), &*r - tok.data());
		}

		return { tok, r };
	}

	std::pair<std::string_view, ItrT> peek() {
		return peekAt(readCursor);
	}

	std::string_view consume() {
		auto state = peek();
		readCursor = state.second;
		return state.first;
	}
};

#endif // ifndef COMPILER_SCANNER_H