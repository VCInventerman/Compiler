#include <fstream>
#include <iostream>
#include <string_view>
#include <string>
#include <filesystem>
#include <variant>
#include <array>

#include "C1.h"

struct PrimitiveType {
	enum Subtype {
		UINT8,
		UINT16,
		UINT32,
		INT32,
		FLOAT,
		STRING,
	};

	Subtype type;
	std::variant<int64_t, double, std::string_view> defaultValue;
};

constexpr PrimitiveType PRIMITIVE_TYPES[] = {
	{ PrimitiveType::UINT8, 0 },
	{ PrimitiveType::UINT16, 0 },
	{ PrimitiveType::UINT32, 0 },
	{ PrimitiveType::INT32, 0 },
	{ PrimitiveType::FLOAT, 0.0 },
	{ PrimitiveType::STRING, "" }
};


struct Type;

struct Object {
	std::vector<Type> members;
};

struct Type {
	std::variant<PrimitiveType, Object> type;
};

struct Declaration {
	std::string_view name;
	std::weak_ptr<void> initializer;
};

struct Scope {
	std::weak_ptr<Scope> parent;

	std::vector<Declaration> declarations;
};

template <typename VarT, typename MatchT>
bool isAnyOf(VarT& var, MatchT& match) {
	for (auto& i : match) {
		if (var == i) {
			return true;
		}
	}
	return false;
}

struct SourcePos {
	std::string_view filename;
	uint64_t line;
	uint64_t offset;

	const char* begin;
	const char* end;
};

SourcePos resolveSourcePos(std::string_view filename, std::string_view file, std::string_view target) {
	SourcePos ptr = { filename, 0, 0, target.data(), target.data() + target.size() };

	uint64_t line = 1;
	uint64_t offset = 1;
	for (const char* i = file.data(); i < file.data() + file.size(); i++) {
		if (i == target.data()) {
			ptr.line = line;
			ptr.offset = offset;
			return ptr;
		}

		if (*i == '\n') {
			line++;
			offset = 1;
		}
		else {
			offset++;
		}
	}

	throw std::range_error("File pointer didn't point inside file!");
}

template <typename ItrT>
struct Parser {
	static constexpr std::array<uint32_t, 3> WHITESPACE = { ' ', '\n', '\t' };
	static constexpr std::array<uint32_t, 6> OPERATORS = { '+', '-', '*', '/', '%', '=' };

	// Begin and end sentinels
	ItrT begin, end;

	// Read cursor
	ItrT r;

	Parser(ItrT _begin, ItrT _end) : begin(_begin), end(_end), r(_begin) {}

	void skipws() {
		if (r == end) { return; }

		while (isAnyOf(*r, WHITESPACE)) {
			r++;

			if (r == end) { return; }
		}
	}

	bool consume(std::string_view& slot) {
		skipws();
		if (r == end) {
			return false;
		}

		slot = std::string_view(&*r, 1);
		r++;

		if (r == end) {
			return true;
		}
		
		while (!isAnyOf(*r, WHITESPACE) && !isAnyOf(*r, OPERATORS)) {
			r++; 
			if (r == end) { return true; }
			slot = std::string_view(slot.data(), &*r);
		}

		// Try to consume an operator if we couldn't find any word tokens
		if (slot.size() == 0 && isAnyOf(*r, OPERATORS)) {
			while (!isAnyOf(*r, WHITESPACE) && isAnyOf(*r, OPERATORS)) {
				r++;
				if (r == end) { return true; }
				slot = std::string_view(slot.data(), &*r);
			}
		}

		return true;
	}

	void parse(std::string_view code) {
		std::string_view tok = "";

		while (consume(tok)) {
			std::cout << std::format("Token {}\n", tok);
		}
	}
};

struct Compiler {
	//todo: unicode
	std::string code;

	std::vector<int> functions;

	Compiler(std::string_view codeFilename) {
		std::fstream codeFile(codeFilename.data(), std::fstream::in);
		auto sz = std::filesystem::file_size(codeFilename);
		code = std::string(sz, '\0');
		codeFile.read(code.data(), code.size());
	}

	void parse() {
		// Produce an AST
		Parser parser(code.cbegin(), code.cend());
		parser.parse(code);
	}
};

int main(int argc, char** argv)
{
	auto defaultFile = "/Users/nickk/dev/Compiler/tests/1.c";

	Compiler compiler(defaultFile);
	compiler.parse();

	return 0;
}
