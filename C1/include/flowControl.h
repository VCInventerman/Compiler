#ifndef COMPILER_FLOWCONTROL_H
#define COMPILER_FLOWCONTROL_H

#include <optional>

#include "forward.h"
#include "function.h"

struct ChildScope : public Expression {
	Scope* scope;

	ChildScope(Scope* scope_) : scope(scope_) {}

	void emitDependency(FuncEmitter& out) override {
		for (auto& i : scope->getExpressions()) {
			i->emitDependency(out);
			out << "\n";
		}
	}
};

struct IfStatement : public Expression {
	Expression* condition = nullptr;

	Scope trueBody;
	Scope falseBody;

	std::string branchPrefix;
	std::string trueBranch;
	std::string falseBranch;
	std::string endOfIfBranch;

	bool hasTrueBranch = false;
	bool hasFalseBranch = false;

	IfStatement(Expression* condition_, Scope* parent_) :
		trueBody("true", Scope::Type::FUNCTION, parent_), falseBody("false", Scope::Type::FUNCTION, parent_) {
		condition = new Cast(condition_, strToType("_condition"));
	}

	void emitDependency(FuncEmitter& out) override {
		branchPrefix = out.nextBranchName();

		endOfIfBranch = branchPrefix + ".end";
		trueBranch = branchPrefix + ".true";
		falseBranch = branchPrefix + ".false";


		// Evaluate condition
		condition->emitDependency(out);
		std::string conditionRegister = condition->getOperand();

		out.indent() << "br i1 " << conditionRegister << ", label %" << (hasTrueBranch ? trueBranch : endOfIfBranch) 
			<< ", label %" << (hasFalseBranch ? falseBranch : endOfIfBranch) << '\n';

		if (hasTrueBranch) {
			// Branch label
			out.indent() << trueBranch << ":\n";

			auto indenter = out.addIndent();

			for (auto& i : trueBody.getExpressions()) {
				i->emitDependency(out);
				out << "\n";
			}

			out.indent() << "br label %" << endOfIfBranch << '\n';
		}

		if (hasFalseBranch) {
			// Branch label
			out.indent() << falseBranch << ":\n";

			auto indenter = out.addIndent();

			for (auto& i : falseBody.getExpressions()) {
				i->emitDependency(out);
				out << "\n";
			}

			out.indent() << "br label %" << endOfIfBranch << '\n';
		}

		out.indent() << endOfIfBranch << ":\n";
	}
};

#endif //ifndef COMPILER_FLOWCONTROL_H