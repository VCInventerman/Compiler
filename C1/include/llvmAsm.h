#ifndef COMPILER_LLVMASM_H
#define COMPILER_LLVMASM_H

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <sstream>

#include "type.h"
#include "function.h"


struct LlvmValue {
	enum class Type {
		NONE,
		VIRTUAL_REGISTER,
		LABEL
	};

	Type type;
	std::any value;
};
	

struct LlvmStackEntry {
	LlvmValue reg;
	int alignBytes;

	LlvmStackEntry(LlvmValue reg_, int alignBytes_) : reg(reg_), alignBytes(alignBytes_) {};
};

class LlvmAsmGenerator {
public:
	std::stringstream llvmAsm;
	std::string_view origFile;

	LlvmAsmGenerator(std::string_view origFile_) : origFile(origFile_) {}

	void generate(std::string_view outFileName, Scope* global) {
		genPreamble();

		for (auto& i : global->getFunctions()) {
			FuncEmitter emitter;
			i->emitFileScope(emitter);
			llvmAsm << emitter.codeOut.str();
		}

		genPostamble();

		if (outFileName == "") {
			std::cout << llvmAsm.str() << '\n';
		}
		else {
			writeToFile(std::string(outFileName));
		}
	}

	void writeToFile(std::string name) {
		std::ofstream file(name);
		file << llvmAsm.str();
	}

	void genPreamble() {
		appendAll(llvmAsm, "; ModuleID = \"", origFile, "\"\n",
			"source_filename = \"", origFile, "\"\n",
			"target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n",
			"target triple = \"x86_64-pc-linux-gnu\"\n",
			"\n",
			"@print_int_fstring = private unnamed_addr constant [4 x i8] c\"%d\\0A\\00\", align 1\n",
			"\n",
			"; Function Attrs: noinline nounwind optnone uwtable\n"
			"define dso_local void @print(i32 %0) #0 {\n",
			"\t%2 = alloca i32, align 4\n",
			"\tstore i32 %0, i32* %2, align 4\n",
			"\t%3 = load i32, i32* %2, align 4\n",
			"\t%4 = call i32(i8*, ...) @printf(i8 * getelementptr inbounds([4 x i8], [4 x i8]* @print_int_fstring, i64 0, i64 0), i32 %3)\n",
			"\tret void\n",
			"}\n\n"
		);
	}

	void genPostamble() {
		appendAll(llvmAsm, R"B(
declare i32 @printf(i8*, ...) #1
attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 10.0.0-4ubuntu1"}
)B");
	}
};

#endif