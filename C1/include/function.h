#ifndef COMPILER_FUNCTION_H
#define COMPILER_FUNCTION_H

#include "expression.h"

struct Function;

class Scope {
public:
	enum class Type {
		GLOBAL,
		NAMESPACE,
		LOCAL,
		CLASS,
		STATEMENT,
		FUNCTION
	};

private:
	std::string_view name = "";
	Type type;
	Scope* parent = nullptr;

	std::vector<Scope*> children;

	std::vector<Function*> functions;
	std::vector<CppType*> types;

	std::vector<Declaration> names;

	std::vector<Expression*> expressions;

public:
	Scope() = delete;
	Scope(std::string_view name_, Type type_) : name(name_), type(type_) {}
	Scope(std::string_view name_, Type type_, Scope* parent_) : name(name_), type(type_), parent(parent_) {}

	void addFunction(Function* fn);

	void addExpression(Expression* exp) {
		expressions.push_back(exp);
	}

	void addChildScope(Scope* scope) {
		children.push_back(scope);
	}

	void addType(CppType* type) {
		types.emplace_back(type);
		names.push_back(Declaration{ type->name, type });
	}

	bool isGlobal() {
		return type == Type::GLOBAL;
	}

	bool isFile() {
		return type == Type::GLOBAL || type == Type::NAMESPACE;
	}

	bool isClass() {
		return type == Type::CLASS;
	}

	std::vector<Function*>& getFunctions() {
		return functions;
	}

	std::vector<Expression*>& getExpressions() {
		return expressions;
	}

	Scope* getParent() {
		return parent;
	}

	std::string_view getName() {
		return name;
	}

	// Get this scope's top level parent
	Scope* global() {
		Scope* itr = this;
		while (itr->type != Type::GLOBAL) {
			if (!itr->parent) {
				throw NULL;
			}

			itr = itr->parent;
		}

		return itr;
	}

	// Searches for a declaration starting from this scope and progressing upwards 
	Declaration* unqualifiedLookup(std::string_view name);

	Declaration* lookup(std::string_view name);
};

struct Function {
	FunctionPrototype decl;

	bool defined = false;
	Scope body;

	Function(Scope* parent) : body("", Scope::Type::FUNCTION, parent) {}
	Function(Scope* parent, FunctionPrototype decl_) : body(decl_.name, Scope::Type::FUNCTION, parent), decl(decl_) {}

	std::string mangleName() {
		auto name = (std::string)decl.name;

		Scope* scope = body.getParent();
		while (!scope->isGlobal()) {
			name = std::string(scope->getName()) + "::" + name;

			scope = scope->getParent();
		}

		return name;
	}

	void emitFileScope(FuncEmitter& out) {
		if (defined) {
			out << "define dso_local " << decl.returnType->llvmName << " @" << mangleName() << "() #4 {\n";

			for (auto& i : body.getExpressions()) {
				i->emitFunctionScope(out);
			}

			if (decl.returnType->name == "void" && typeid(body.getExpressions().back()) != typeid(Return)) {
				// No return is required at the end if the return type is void
				Return ret;
				ret.emitFunctionScope(out);
			}
			else if (decl.name == "main" && typeid(body.getExpressions().back()) != typeid(Return)) {
				// The end of main implicitly returns 0
				Return ret;
				ret.ret = new IntegerLiteral(0);
				ret.emitFunctionScope(out);
			}

			out << "}\n";
		}
		else {
			out << "declare dso_local " << decl.returnType->llvmName << " @" << mangleName() << "() #4\n";
		}
	}

	std::string_view getName() {
		return decl.name;
	}
};


void Scope::addFunction(Function* fn) {
	functions.emplace_back(fn);

	Declaration decl;
	decl.name = fn->decl.name;
	decl.data = fn;
	names.push_back(decl);
}

// Searches for a declaration starting from this scope and progressing upwards 
Declaration* Scope::unqualifiedLookup(std::string_view name) {
	for (auto& i : names) {
		if (i.name == name) {
			return &i;
		}
	}

	if (parent) {
		return parent->unqualifiedLookup(name);
	}
	else {
		std::cout << "Failed to lookup name " << name << '\n';
	}

	return nullptr;
}

Declaration* Scope::lookup(std::string_view name) {
	if (name == "") {
		std::cout << "Looking up empty string!\n";
		throw NULL;
	}

	if (name.substr(0, 2) == "::") {
		return global()->lookup(name.substr(2));
	}

	return unqualifiedLookup(name);
}

#endif // ifndef COMPILER_FUNCTION_H