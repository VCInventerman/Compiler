#include "util.h"
#include "expression.h"
#include "function.h"

void FunctionCall::emitDependency(FuncEmitter& out) {
	for (auto& i : arguments) {
		i->emitDependency(out);
	}

	if (_fn->decl.returnType->getName() != "void") {
		_retReg = out.nextReg();
		out.indent() << "%" << _retReg << " = ";
	}
	else {
		out.indent();
	}

	out << "call " << _fn->decl.returnType->getLlvmName() << " @" << _fn->mangleName() << "(";

	for (auto& i : arguments) {
		out << i->getResultType()->getLlvmName() << " " << i->getOperand();

		if (&i != &arguments.back()) {
			out << ", ";
		}
	}

	out << ")\n";
}

std::string FunctionCall::getOperand() {
	return buildStr("%", _retReg);
}

CppType* FunctionCall::getResultType() {
	return _fn->decl.returnType;
}