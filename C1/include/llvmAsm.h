#ifndef COMPILER_LLVMASM_H
#define COMPILER_LLVMASM_H

#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <sstream>

#include "ast.h"
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

	int virtualRegisterNum = 0;
	int freeRegisters = 0;
	std::vector<LlvmValue> loadedRegisters;

	int llvmRegisterNumber = 0;

	LlvmAsmGenerator(std::string_view origFile_) : origFile(origFile_) {}

	int getNextLocalVirtualRegister() {
		return ++llvmRegisterNumber;
	}

	int updateFreeRegisterCount(int delta) {
		freeRegisters += delta;
		return freeRegisters - delta;
	}

	int getFreeRegisterCount() {
		return updateFreeRegisterCount(0);
	}

	std::vector<LlvmStackEntry> determineBinaryStackAllocation(AstNode* root) {
		std::vector<LlvmStackEntry> leftEntry;
		std::vector<LlvmStackEntry> rightEntry;

		if (root->leftChild || root->rightChild) {
			if (root->leftChild) {
				leftEntry = determineBinaryStackAllocation(root->leftChild.get());
			}
			if (root->rightChild) {
				rightEntry = determineBinaryStackAllocation(root->rightChild.get());
			}

			leftEntry.insert(leftEntry.end(), rightEntry.begin(), rightEntry.end());
			return leftEntry;
		}
		else {
			LlvmStackEntry outEntry(LlvmValue{ LlvmValue::Type::VIRTUAL_REGISTER, getNextLocalVirtualRegister() }, 4);

			updateFreeRegisterCount(1);

			return { outEntry };
		}
	}

	void binaryStackAllocation(std::vector<LlvmStackEntry>& entries) {
		for (auto& i : entries) {
			appendAll(llvmAsm, "\t%", std::any_cast<int>(i.reg.value), " = alloca i32, align ", i.alignBytes, "\n");
		}
	}

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


	void generateOld(std::string_view outFileName, std::vector<std::unique_ptr<AstNode>>& statements) {
		//origFile = root->original.code.filename;

		genPreamble();

		for (auto& i : statements) {
			auto stack = determineBinaryStackAllocation(i.get());
			binaryStackAllocation(stack);

			auto printVr = astToLlvm(i.get());

			printInt(printVr);
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

	void printInt(LlvmValue reg) {
		getNextLocalVirtualRegister(); // uses a register for some reason
		appendAll(llvmAsm, "\tcall i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @print_int_fstring , i32 0, i32 0), i32 %", std::any_cast<int>(reg.value), ")\n");
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

	bool isBinaryArithmetic(TokenType type) {
		return type >= TokenType::PLUS && type <= TokenType::DIV;
	}

	bool isTerminal(TokenType type) {
		return type == TokenType::PRIMITIVE;
	}

	bool isPrimitiveSame(LlvmValue& lhs, LlvmValue rhs) {
		if (rhs.value.type() != lhs.value.type()) {
			return false;
		}

		if (rhs.value.type() == typeid(int)) {
			return std::any_cast<int>(rhs.value) == std::any_cast<int>(lhs.value);
		}

		throw NULL;
	}

	std::vector<LlvmValue> ensureRegistersLoaded(std::vector<LlvmValue> toCheck) {
		std::vector<bool> found;
		found.resize(toCheck.size(), false);

		for (auto& loadedRegister : loadedRegisters) {
			int i = 0;
			for (auto& potentialRegister : toCheck) {
				if (isPrimitiveSame(potentialRegister, loadedRegister)) {
					found[i] = true;
				}

				i++;
			}
		}

		// Exit if all registers have already had their primitives loaded
		if (std::all_of(found.cbegin(), found.cend(), [](bool i) { return i; })) {
			return toCheck;
		}

		std::vector<LlvmValue> loaded;
		for (int i = 0; i < toCheck.size(); i++) {
			if (!found[i]) {
				int newRegNum = getNextLocalVirtualRegister();
				loaded.push_back(LlvmValue{ LlvmValue::Type::VIRTUAL_REGISTER, newRegNum });

				appendAll(llvmAsm, "\t%", newRegNum, " = load i32, i32* %",
					std::any_cast<int>(toCheck[i].value), '\n');

				loadedRegisters.push_back(loaded.back());
			}
			else {
				loaded.push_back(toCheck[i]);
			}
		}

		return loaded;
	}

	LlvmValue llvmBinaryArithmetic(Token token, LlvmValue leftVr, LlvmValue rightVr) {
		LlvmValue outVr; // Output virtual register
		int outReg = getNextLocalVirtualRegister();

		if (token.type == TokenType::PLUS) {
			appendAll(llvmAsm, "\t%", outReg, " = add i32 %", std::any_cast<int>(leftVr.value), ", %", std::any_cast<int>(rightVr.value), "\n");
		}
		if (token.type == TokenType::MINUS) {
			appendAll(llvmAsm, "\t%", outReg, " = sub nsw i32 %", std::any_cast<int>(leftVr.value), ", %", std::any_cast<int>(rightVr.value), "\n");
		}
		if (token.type == TokenType::MULT) {
			appendAll(llvmAsm, "\t%", outReg, " = mul nsw i32 %", std::any_cast<int>(leftVr.value), ", %", std::any_cast<int>(rightVr.value), "\n");
		}
		if (token.type == TokenType::DIV) {
			appendAll(llvmAsm, "\t%", outReg, " = udiv i32 %", std::any_cast<int>(leftVr.value), ", %", std::any_cast<int>(rightVr.value), "\n");
		}

		outVr = LlvmValue{ LlvmValue::Type::VIRTUAL_REGISTER, outReg };

		loadedRegisters.push_back(outVr);

		return outVr;
	}

	/*LlvmValue storeConstant(std::string_view register, std::any value) {
		const std::type_info& type = value.type();

		
	}*/

	LlvmValue llvmStoreConstant(std::any val) {
		if (val.type() != typeid(int)) {
			throw NULL;
		}

		appendAll(llvmAsm, "\tstore i32 ", std::any_cast<int>(val), ", i32* %",
			updateFreeRegisterCount(-1), "\n");

		return LlvmValue{ LlvmValue::Type::VIRTUAL_REGISTER, getFreeRegisterCount() + 1 };
	}

	//void llvmPrintInt(LlvmValue reg) {
	//	appendAll(llvmAsm, "\tcall i32(i8*, ...) @printf(i8 * getelementptr inbounds([4 x i8], [4 x i8] * @print_int_fstring, i32 0, i32 0), i32%", std::any_cast<int>(reg.value), "\n");
	//}

	LlvmValue astToLlvm(AstNode* root) {
		LlvmValue leftVr;
		LlvmValue rightVr;

		if (root->leftChild) {
			leftVr = astToLlvm(root->leftChild.get());
		}
		if (root->rightChild) {
			rightVr = astToLlvm(root->rightChild.get());
		}

		if (isBinaryArithmetic(root->tok.type)) {
			leftVr = ensureRegistersLoaded({ leftVr })[0];
			rightVr = ensureRegistersLoaded({ rightVr })[0];
			return llvmBinaryArithmetic(root->tok, leftVr, rightVr);
		}
		else if (isTerminal(root->tok.type)) {
			if (root->tok.type == TokenType::PRIMITIVE) {
				return llvmStoreConstant(root->tok.defaultValue);
			}
		}
		else if (root->tok.type == TokenType::PRINT) {
			rightVr = ensureRegistersLoaded({ rightVr })[0];
			//llvmPrintInt(rightVr);
			return {};
		}
		else {
			throw NULL;
		}

		return LlvmValue{ LlvmValue::Type::NONE, NULL };
	}
};

#endif