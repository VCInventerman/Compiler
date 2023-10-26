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

	inline void addFunction(Function* fn);

	inline void addExpression(Expression* exp) {
		expressions.push_back(exp);
	}

	inline void addChildScope(Scope* scope) {
		children.push_back(scope);
	}

	inline void addType(CppType* type) {
		types.emplace_back(type);
		names.push_back(Declaration{ type->name, type });
	}

	inline void addDeclaration(Declaration decl) {
		names.push_back(decl);
	}

	inline bool isGlobal() {
		return type == Type::GLOBAL;
	}

	inline bool isFile() {
		return type == Type::GLOBAL || type == Type::NAMESPACE;
	}

	inline bool isClass() {
		return type == Type::CLASS;
	}

	inline std::vector<Function*>& getFunctions() {
		return functions;
	}

	inline std::vector<Expression*>& getExpressions() {
		return expressions;
	}

	inline Scope* getParent() {
		return parent;
	}

	inline std::string_view getName() {
		return name;
	}

	// Get this scope's top level parent
	inline Scope* global() {
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
	inline Declaration* unqualifiedLookup(std::string_view name);

	inline Declaration* lookup(std::string_view name);
};

struct Function {
	FunctionPrototype decl;

	bool defined = false;
	Scope body;

	Function(Scope* parent) : body("", Scope::Type::FUNCTION, parent) {}
	Function(Scope* parent, FunctionPrototype decl_) : body(decl_.name, Scope::Type::FUNCTION, parent), decl(decl_) {}

	inline std::string mangleName() {
		auto name = (std::string)decl.name;

		Scope* scope = body.getParent();
		while (!scope->isGlobal()) {
			name = std::string(scope->getName()) + "::" + name;

			scope = scope->getParent();
		}

		return name;
	}

	inline void emitFileScope(FuncEmitter& out) {
		if (mangleName() == "print") { return; }

		out.regCnt = decl.arguments.size() == 0;

		if (defined) {
			out << "define dso_local " << decl.returnType->llvmName << " @" << mangleName() << " (";
			
			for (int i = 0; i < decl.arguments.size(); i++) {
				auto& arg = decl.arguments[i];

				out << arg.type->llvmName << " %" << out.nextReg();
				if (i != decl.arguments.size() - 1) {
					out << ", ";
				}
			}

			out << ") #0" << "{\n";

			for (auto& i : decl.arguments) {
				out << i.type;
				out << " %" << out.nextReg();
			}
			
			//"() #4 {\n";

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
			out << "declare dso_local " << (decl.returnType ? decl.returnType->llvmName : "void") << " @" << mangleName() << " (";

			for (int i = 0; i < decl.arguments.size(); i++) {
				auto& arg = decl.arguments[i];

				out << arg.type->llvmName << " %" << out.nextReg();
				if (i != decl.arguments.size() - 1) {
					out << ", ";
				}
			}

			out << ") #0\n";
		}
	}

	inline std::string_view getName() {
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