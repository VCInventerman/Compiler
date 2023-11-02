#ifndef COMPILER_FLOWCONTROL_H
#define COMPILER_FLOWCONTROL_H

#include <optional>

#include "forward.h"
#include "function.h"

struct IfStatement : Expression {
	Expression* _condition;

	Scope _trueBody;
	Scope _falseBody;

	IfStatement(Expression* condition_, Scope* parent_) : _condition(condition_), 
		_trueBody("true", Scope::Type::FUNCTION, parent_), _falseBody("false", Scope::Type::FUNCTION, parent_) {}


};

#endif //ifndef COMPILER_FLOWCONTROL_H