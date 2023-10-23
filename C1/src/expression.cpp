#include "expression.h"
#include "function.h"

void FunctionCall::emitFunctionScope(FuncEmitter& out) {
	for (auto& i : arguments) {
		i->emitFunctionScope(out);
	}

	_retReg = out.nextReg(); // Needs to use a register, even if we don't put a value in it

	if (_fn->decl.returnType->name != "void") {
		out << "\t%" << _retReg << " = ";
	}

	out << "call " << _fn->decl.returnType->llvmName << " @" << _fn->mangleName() << "(";

	for (auto& i : arguments) {
		out << i->getOperandType() << " ";
		i->emitOperand(out);
	}

	out << ")\n";
}

void FunctionCall::emitOperand(FuncEmitter& out) {
	out << "%" << _retReg;
}

std::string FunctionCall::getOperandType() {
	throw _fn->decl.returnType->llvmName;
}