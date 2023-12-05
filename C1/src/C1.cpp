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
		standard->addType(new CppType("nullptr_t")); // std::nullptr_t, the type of nullptr
		globalScope.addChildScope(standard);

		Function* print = new Function(&globalScope);
		print->decl = FunctionPrototype{ "print", { new FunctionArgument{ "target", strToType("int") } }, strToType("void") };
		globalScope.addFunction(print);

		Function* printStr = new Function(&globalScope);
		printStr->decl = FunctionPrototype{ "puts", { new FunctionArgument{ "target", strToType("char*") } }, strToType("int") };
		globalScope.addFunction(printStr);

		Function* malloc = new Function(&globalScope);
		malloc->decl = FunctionPrototype{ "malloc", { new FunctionArgument{ "size", strToType("int") } }, strToType("void*") };
		globalScope.addFunction(malloc);

		Function* free = new Function(&globalScope);
		free->decl = FunctionPrototype{ "free", { new FunctionArgument{ "ptr", strToType("void*") } }, strToType("void") };
		globalScope.addFunction(free);
	}

	void loadFile(std::string_view codeFilename_) {
		int ret = system(buildStr("clang -E ", codeFilename_, " -o temp.inc").data());
		if (ret) {
			system(buildStr("cl /P /Fitemp.inc ", codeFilename_).data());
		}

		// Need to ignore comments for this to work
		//codeFilename = "temp.inc";
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
	auto defaultFile = "C:/Users/nickk/dev/Compiler/tests/test_1.cpp";
	auto defaultOut = "C:/Users/nickk/dev/Compiler/build/out.ll";

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
