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
#include "util.h"
#include "parser.h"
#include "llvmAsm.h"
#include "type.h"
#include "HashMap.h"

struct Compiler {
	//todo: unicode
	std::string sourceCode;
	std::string_view codeFilename;

	Scope globalScope;
	
	Compiler() :
		globalScope("::", Scope::Type::GLOBAL)
	{
		makeGlobalScope();
	}

	void makeGlobalScope() {
		Scope* standard = new Scope("std", Scope::Type::NAMESPACE);
		standard->addType(new CppType{ "nullptr_t", "i32*" }); // std::nullptr_t, the type of nullptr
		globalScope.addChildScope(standard);

		Function* print = new Function(&globalScope);
		print->decl = FunctionPrototype{ "print", { { "target", strToType("int") } }, strToType("void") };
		globalScope.addFunction(print);
	}

	void loadFile(std::string_view codeFilename_) {
		codeFilename = codeFilename_;

		std::fstream codeFile(codeFilename.data(), std::fstream::in);
		auto sz = std::filesystem::file_size(codeFilename);

		sourceCode = std::string(sz, '\0');
		codeFile.read(sourceCode.data(), sourceCode.size());

		int i = (int)sourceCode.size() - 1;
		while (i >= 0) {
			if (sourceCode[i] == '\0') {
				sourceCode.pop_back();
			}
			else {
				break;
			}

			i--;
		}
	}

	void parse() {
		// Produce an AST
		Parser parser(sourceCode, codeFilename);

		parser.parse(&globalScope);
	}

	void generateIr(std::string_view outputFilename) {
		LlvmAsmGenerator gen(codeFilename);
		gen.generate(outputFilename, &globalScope);
	}
};

int main(int argc, char** argv)
{
	auto defaultFile = "/Users/nickk/dev/Compiler/tests/test_1.cpp";
	auto defaultOut = "/Users/nickk/dev/Compiler/build/out.ll";

	if (argc >= 2) {
		defaultFile = argv[1];
	}

	if (argc >= 3) {
		defaultOut = argv[2];
	}

	Compiler compiler;
	compiler.loadFile(defaultFile);
	compiler.parse();
	compiler.generateIr(defaultOut);

	return 0;
}
