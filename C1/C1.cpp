#include <fstream>
#include <iostream>
#include <string_view>
#include <string>
#include <filesystem>
#include <variant>
#include <array>
#include <functional>
#include <any>
#include <algorithm>

#include "token.h"
#include "ast.h"
#include "util.h"
#include "parser.h"


struct Compiler {
	//todo: unicode
	std::string code;
	std::string_view codeFilename;

	std::vector<int> functions;

	Compiler(std::string_view codeFilename_) : codeFilename(codeFilename_) {
		std::fstream codeFile(codeFilename.data(), std::fstream::in);
		auto sz = std::filesystem::file_size(codeFilename);

		code = std::string(sz, '\0');
		codeFile.read(code.data(), code.size());
	}

	void parse() {
		// Produce an AST
		Parser parser(code, { codeFilename });

		// Instantly execute it
		parser.parse();
	}
};

int main(int argc, char** argv)
{
	auto defaultFile = "/Users/nickk/dev/Compiler/tests/1.c";

	Compiler compiler(defaultFile);
	compiler.parse();

	return 0;
}
