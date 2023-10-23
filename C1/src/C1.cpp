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
#include "type.h"
#include "HashMap.h"

std::string mangle(Declaration* decl) {
	std::string out = "_Z";
	out.append(std::to_string((uintptr_t)decl));
	return out;
}

struct Compiler {
	//todo: unicode
	std::string code;
	std::string_view codeFilename;
	std::string_view outputFilename;

	HashMap<int> symbolTable;

	Scope globalScope;
	
	Compiler(std::string_view codeFilename_, std::string_view outputFilename_) :
		codeFilename(codeFilename_), outputFilename(outputFilename_),
		globalScope("::", Scope::Type::GLOBAL)
	{
		std::fstream codeFile(codeFilename.data(), std::fstream::in);
		auto sz = std::filesystem::file_size(codeFilename);

		code = std::string(sz, '\0');
		codeFile.read(code.data(), code.size());
	}

	void makeGlobalScope() {
		Scope* standard = new Scope("std", Scope::Type::NAMESPACE);
		standard->addType(new CppType{ "nullptr_t", "i32*" }); // std::nullptr_t, the type of nullptr
		globalScope.addChildScope(standard);

		Function* print = new Function(&globalScope);
		print->decl = FunctionPrototype{ "print", { { "target", strToType("int") } }, strToType("void") };
		globalScope.addFunction(print);
	}

	void parse() {
		makeGlobalScope();

		// Produce an AST
		Parser parser(code, codeFilename);

		parser.parse(&globalScope);

		LlvmAsmGenerator gen(codeFilename);
		gen.generate(outputFilename, &globalScope);
		//gen.generate(outputFilename, func.statements);
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
