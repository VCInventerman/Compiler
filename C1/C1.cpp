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
#include "llvmAsm.h"
#include "statement.h"
#include "type.h"


struct Compiler {
	//todo: unicode
	std::string code;
	std::string_view codeFilename;
	std::string_view outputFilename;

	std::vector<int> functions;

	
	Compiler(std::string_view codeFilename_, std::string_view outputFilename_) : 
		codeFilename(codeFilename_), outputFilename(outputFilename_) 
	{
		std::fstream codeFile(codeFilename.data(), std::fstream::in);
		auto sz = std::filesystem::file_size(codeFilename);

		code = std::string(sz, '\0');
		codeFile.read(code.data(), code.size());
	}

	void parse() {
		// Produce an AST
		Parser parser(code, codeFilename);

		auto func = parser.parse();

		Scope globalScope;
		Scope singleFunScope;
		singleFunScope.parent = &globalScope;

		LlvmAsmGenerator gen(codeFilename);
		gen.generate(outputFilename, func.statements);
	}
};

int main(int argc, char** argv)
{
	auto defaultFile = "/Users/nickk/dev/Compiler/tests/llvm_test_1"; // "tests/parse_test_1";
	auto defaultOut = "/Users/nickk/dev/Compiler/out.ll";

	if (argc >= 2) {
		defaultFile = argv[1];
	}

	if (argc >= 3) {
		defaultOut = argv[2];
	}

	Compiler compiler(defaultFile, defaultOut);
	compiler.parse();

	return 0;
}
