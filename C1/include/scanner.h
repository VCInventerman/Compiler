#ifndef COMPILER_SCANNER_H
#define COMPILER_SCANNER_H

#include <string_view>
#include <array>
#include <functional>
#include <algorithm>
#include <regex>

#include "error.h"
#include "util.h"
#include "token.h"

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

enum class SIMPLE_TYPE_SPECIFIERS {
	CHAR,
	CHAR8_T,
	CHAR16_T,
	CHAR32_T,
	WCHAR_T,
	BOOL,
	SHORT,
	INT,
	LONG,
	SIGNED,
	UNSIGNED,
	FLOAT,
	DOUBLE,
	VOID
};
constexpr std::string_view SIMPLE_TYPE_SPECIFIERS_STR[] = {
	"char8_t",
	"char16_t",
	"char32_t",
	"wchar_t",
	"bool",
	"short",
	"int",
	"long",
	"signed",
	"unsigned",
	"float",
	"double",
	"void"
};

struct LiteralParser {
	static bool isDecimal(char c) {
		return c >= '0' && c <= '9';
	}
	static bool isHex(char c) {
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}
	static bool isOctal(char c) {
		return c >= '0' && c <= '8';
	}
	static bool isNumBase(char c, int base) {
		constexpr int MAX_REPRESENTABLE = '9' - '0' + 'z' - 'a';

		eassert(base < MAX_REPRESENTABLE);

		// Bring capital letters up to lowercase
		c = (c >= 'a' && c <= 'z') ? c + 0x20 : c;

		if (base < 10) {
			return c >= '0' && c <= ('0' + base);
		}
		else if (c >= '0' && c <= '9') {
			return true;
		}
		else if (c >= 'a' && c <= ('f' - base + 10)) {
			return true;
		}

		return false;
	}

	static int strToNum(const char*& r, const char* end, int base = 10) {
		int val = 0;

		while (isNumBase(*r, base) && r != end) {
			val = val * base;

			if (*r >= '0' && *r <= '9') {
				val += *r - '0';
			}
			else if (*r >= 'a' && *r <= 'f') {
				val += *r - 'a';
			}
			else {
				val += *r - 'A';
			}

			++r;
		}

		return val;
	}

	static bool testRegex(std::string_view regexStr, const char* r, const char* end) {
		std::regex regex(regexStr.data(), regexStr.size());
		return std::regex_search(r, end, regex);
	}

	static bool parseDecimalLiteral(const char*& r, const char* end, LiteralContainer& container) {
		// At least a nonzero digit, decimal digits, and a suffix
		if (!testRegex("^[1-9][0-9]*(?:(?:ll)|(?:LL)|(?:[uUlLzZ]))?", r, end)) {
			return false;
		}

		int64_t num = strToNum(r, end, 10);
		container = num;

		if (r == end) { return true; }

		//todo: handle suffix
		if (isAnyOf(*r, { 'u', 'U', 'l', 'L', 'z', 'Z' })) {
			r++;

			if (r != end && (*r == 'l' || *r == 'L')) {
				r++;
			}
		}

		return true;
	}

	static bool parseOctalLiteral(const char*& r, const char* end, LiteralContainer& container) {
		// A zero, decimal digits, and a suffix
		if (!testRegex("^0[0-7]*(?:(?:ll)|(?:LL)|(?:[uUlLzZ]))?", r, end)) {
			return false;
		}

		int64_t num = strToNum(r, end, 8);
		container = num;

		if (r == end) { return true; }

		//todo: handle suffix
		if (isAnyOf(*r, { 'u', 'U', 'l', 'L', 'z', 'Z' })) {
			r++;

			if (r != end && (*r == 'l' || *r == 'L')) {
				r++;
			}
		}

		return true;
	}

	static bool parseCharacterLiteral(const char*& r, const char* end, LiteralContainer& container) {
		// Capture a sequence like 'a' or '\?'
		if (!testRegex(R"(^'[a-zA-Z0-9]'|'\\[\\\"\?abfnrtv0]')", r, end)) {
			return false;
		}

		constexpr std::pair<std::string_view, char> SIMPLE_ESCAPE_SEQUENCES[12] = {
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
			{ "\\0", 0x00 },
		};

		r++; // '

		if (r[1] == '\\') {
			auto simpleReplacement = std::find_if(std::begin(SIMPLE_ESCAPE_SEQUENCES), std::end(SIMPLE_ESCAPE_SEQUENCES),
				[&](const std::pair<std::string_view, char> i) {
					return i.first[1] == r[1];
				});
			container = int(simpleReplacement->second);
			r += 2;
		}
		else {
			container = int(r[0]);
			r++;
		}

		r++; // '
		return true;
	}

	static bool parseStringLiteral(const char*& r, const char* end, LiteralContainer& container) {
		if (!testRegex(R"(^"[a-zA-Z\\0-9]*")", r, end)) {
			return false;
		}

		constexpr std::pair<std::string_view, char> SIMPLE_ESCAPE_SEQUENCES[12] = {
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
			{ "\\0", 0x00 },
		};

		std::string out;

		for (; r < end; r++) {
			int c = *r;

			auto next = [&]() { return (r < end - 1) ? (int)(*(r + 1)) : EOF; };

			auto simpleReplacement = std::find_if(std::begin(SIMPLE_ESCAPE_SEQUENCES), std::end(SIMPLE_ESCAPE_SEQUENCES),
				[&](const std::pair<std::string_view, char> i) {
					return i.first[1] == next();
				});
			if (c == '\\' && simpleReplacement != std::end(SIMPLE_ESCAPE_SEQUENCES)) {
				out += simpleReplacement->second;

				r++;
			}
			else {
				out += c;
			}
		}

		out += '\0';
		container = out;
		return true;
	}

	static LiteralContainer parseLiteral(const char*& r, const char* end) {
		LiteralContainer slot;

		if (parseDecimalLiteral(r, end, slot)) {}
		else if (parseOctalLiteral(r, end, slot)) {}
		else if (parseCharacterLiteral(r, end, slot)) {}
		else if (parseStringLiteral(r, end, slot)) {}
		else {
			return LiteralContainerEmpty{};
		}
		
		return slot;
	}
};


class Scanner {
public:
	using ItrT = const char*;

	// Begin and end sentinels
	std::string_view code;

	ItrT readCursor;

	// String identifying where the sequence being scanned came from, such as a filename
	std::string_view sourceName;

	Scanner(std::string_view code_, std::string_view source_) : code(code_), sourceName(source_) {
		readCursor = &*code.begin();
	}

	SourcePos makeSourcePos(ItrT start, ItrT end) {
		return { sourceName, code, start, end };
	}

	SourcePos makeSourcePos(ItrT start) {
		return { sourceName, code, start, start };
	}

	ItrT skipws() {
		return skipWhiteSpace(readCursor, code.data() + code.size());
	}

	// Observes a token starting at the argument readCursor
	// Returns: the token and the state of the read cursor if the token is kept
	std::pair<Token, ItrT> peekAt(ItrT readCursor) {
		ItrT r = skipws();
		ItrT lastR = r;

		Token tok = { makeSourcePos(readCursor), TokenType::UNKNOWN, "" };

		if (r == code.data() + code.size()) {
			return { tok, r };
		}

		LiteralContainer literal = LiteralParser::parseLiteral(r, code.data() + code.size());
		tok.str = std::string_view(lastR, r - lastR);

		if (int64_t* val = std::get_if<int64_t>(&literal)) {
			tok.type = TokenType::INTEGER_LITERAL;
			tok.value = *val;
			return { tok, r };
		}
		else if (double* val = std::get_if<double>(&literal)) {
			tok.type = TokenType::FLOAT_LITERAL;
			tok.value = *val;
			return { tok, r };
		}
		else if (std::string* val = std::get_if<std::string>(&literal)) {
			tok.type = TokenType::STRING_LITERAL;
			tok.value = *val;
			return { tok, r };
		}

		// Function evaluating whether a string matches the pattern started by its first character
		std::function<bool()> check;

		char firstChar = *r;
		tok.str = std::string_view(&*r, 1);
		r++;

		if (IS_ARRAY_SUBMEMBER_ANY_OF(OPERATOR_TRAITS, firstChar, str[0])) {
			tok.type = TokenType::Operator;
			check = [&tok, &r]() {
				// Consume until there is not at least one operator with the same beginning characters
				return std::any_of(std::begin(OPERATOR_TRAITS), std::end(OPERATOR_TRAITS),
					[&tok](OperatorTrait test) {
						return test.str.starts_with(tok.str);
					});
			};
		}
		// An identifier may start with an alphabetical character or an underscore. This will also match keywords.
		else if ((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z') || (firstChar == '_')) {
			tok.type = TokenType::IDENTIFIER;
			check = [&tok]() {
				// After the first character, numbers are allowed too
				char c = tok.str[tok.str.size() - 1];
				return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
			};
		}
		else {
			throw SourceError("Unexpected token beginning", makeSourcePos(r, r));
		}

		while (true) {
			tok.str = std::string_view(tok.str.data(), &*r - tok.str.data());

			bool matches = check();

			if (matches) {
				if (r == code.data() + code.size()) { return { tok, r }; }
				r++;
				continue;
			}
			else {
				tok.str.remove_suffix(1);
				r--;
				return { tok, r };
			}
		}
	}

	// Consume a token, without advancing readCursor
	// Returns: the token and the state of the read cursor if the token is kept
	std::pair<Token, ItrT> peek() {
		return peekAt(readCursor);
	}

	// Consume a token and return it
	Token consume() {
		auto state = peek();
		readCursor = state.second;
		return state.first;
	}

	bool isValidIdentifier(std::string_view str) {
		if (str.size() == 0) {
			return false;
		}

		char firstChar = str[0];

		if ((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z') || (firstChar == '_')) {
			return std::all_of(str.begin() + 1, str.end(), [](uint32_t c) {
				// After the first character, numbers are allowed too
				return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
			});
		}
		else {
			return false;
		}
	}

	// Stores a snapshot of scanner state and restores it upon leaving scope
	struct VirtualScanner {
		ItrT _orig;
		Scanner* _scanner;
		bool _keep = false;

		void keep() {
			_keep = true;
		}

		~VirtualScanner() {
			if (!_keep) {
				_scanner->readCursor = _orig;
			}
		}
	};

	// Returns a snapshot of parser state and restores it when it leaves scope, unless .keep() is called
	VirtualScanner startVirtualScan() {
		return { readCursor, this };
	}
};

#endif // ifndef COMPILER_SCANNER_H