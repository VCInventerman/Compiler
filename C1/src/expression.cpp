#include "util.h"
#include "expression.h"
#include "function.h"

void FunctionCall::emitDependency(FuncEmitter& out) {
	for (auto& i : arguments) {
		i->emitDependency(out);
	}

	if (_fn->decl.returnType->getName() != "void") {
		out << "\t%" << _retReg << " = ";
		_retReg = out.nextReg();
	}

	out << "\tcall " << _fn->decl.returnType->getLlvmName() << " @" << _fn->mangleName() << "(";

	for (auto& i : arguments) {
		out << i->getResultType()->getLlvmName() << " " << i->getOperand();
	}

	out << ")\n";
}

std::string FunctionCall::getOperand() {
	return buildStr("%", _retReg);
}

CppType* FunctionCall::getResultType() {
	throw _fn->decl.returnType->getLlvmName();
}