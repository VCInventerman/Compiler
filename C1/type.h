#ifndef COMPILER_TYPE_H
#define COMPILER_TYPE_H

#include <vector>
#include <string_view>
#include <variant>
#include <memory>

#include "ast.h"

enum class DataModel {
	// Bytes per int/long/pointer
	LP32, // 2/4/4
	ILP32, // 4/4/4

	LLP64, // 4/4/8
	LP64, // 4/8/8

	ILP64, // 8/8/8

	ARDUINO_OLD, // 2/4/4
	ARDUINO, // 4/8/4
};

struct PrimitiveType {
	enum Subtype {
		UINT8,
		UINT16,
		UINT32,
		INT32,
		FLOAT,
		STRING,
	};

	Subtype type;
	std::variant<int64_t, double, std::string_view> defaultValue;
};

constexpr PrimitiveType PRIMITIVE_TYPES[] = {
	{ PrimitiveType::UINT8, int64_t(0) },
	{ PrimitiveType::UINT16, int64_t(0) },
	{ PrimitiveType::UINT32, int64_t(0) },
	{ PrimitiveType::INT32, int64_t(0) },
	{ PrimitiveType::FLOAT, 0.0 },
	{ PrimitiveType::STRING, "" }
};


struct Type;

struct Object {
	std::vector<Type> members;
};

struct Type {
	std::variant<PrimitiveType, Object> type;
};

struct Declaration {
	std::string_view name;
	std::unique_ptr<Expression> initializer;
};

struct Identifier {
	std::string_view name;
};

struct CppType {
	// Applies to class, union, enum, typedef, and type alias
	std::string name;
	std::string llvmName;

	bool isConst = false;
	bool isVolatile = false;

	bool isFundamental;
};

struct CppTypeVoid {
	static CppType make() { return { "void", "void" }; }
};

struct CppTypeNullptrT {
	std::string name() {
		return "nullptr_t";
	}

	int width() {
		return 0;
	}
};

struct CppTypeIntegral {
	enum class INTEGRAL_TYPES {
		BOOL,

		// Narrow characters
		CHAR,
		CHAR8_T,

		// Wide characters
		CHAR16_T,
		CHAR32_T,
		WCHAR_T,

		// Signed integers
		SIGNED_CHAR,
		SHORT,
		INT,
		LONG,
		LONG_LONG,

		// Unsigned integers
		UNSIGNED_CHAR,
		UNSIGNED_SHORT,
		UNSIGNED,
		UNSIGNED_LONG,
		UNSIGNED_LONG_LONG
	};

	static CppType make(std::string_view base) {
		if (base == "bool") { return CppType{ "bool", "i1" }; }
		else if (base == "int") { return CppType{ "bool", "i32" }; }
		else if (base == "float") { return CppType{ "bool", "f32" }; }
		else { return CppTypeVoid::make(); }
	}
};

struct Function;

struct Scope {
	enum class Type {
		GLOBAL,
		NAMESPACE,
		LOCAL,
		CLASS,
		STATEMENT,
		FUNCTION
	};

	Type type;
	Scope* parent = nullptr;

	std::vector<Scope> children;

	std::vector<Function> functions;
	std::vector<CppType> types;

	std::vector<std::string_view> names;


	Scope* global() {
		Scope* itr = this;
		while (itr->type != Type::GLOBAL) {
			if (!itr->parent) {
			}

			itr = itr->parent;
		}
	}

	// Searches for a declaration starting from this scope and progressing upwards 
	/*Declaration* unqualifiedLookup(std::string_view name) {
		for (auto& i : declarations) {
			if (i.name == name) {
				return &i;
			}
		}

		if (parent) {
			return parent->unqualifiedLookup(name);
		}
	}

	Declaration* lookup(std::string name) {
		if (name.substr(0, 2) == "::") {
			global()->lookup(name.substr(2));
		}

		return unqualifiedLookup(name);
	}*/
};

struct Function {
	// prototype?

	std::string_view name;

	CppType returnType;

	Scope bodyScope;
	std::vector<std::unique_ptr<Expression>> statements;

	std::string mangleName() {
		return std::string(name);
	}

	void emitFileScope(FuncEmitter& out) {
		out << "define dso_local " << returnType.llvmName << " @" << mangleName() << "() #4 {\n";

		for (auto& i : statements) {
			i->emitFunctionScope(out);
		}

		if (returnType.name == "void" && typeid(statements.back().get()) != typeid(Return)) {
			// No return is required at the end if the return type is void
			Return ret;
			ret.emitFunctionScope(out);
		}
		else if (name == "main" && typeid(statements.back().get()) != typeid(Return)) {
			// The end of main implicitly returns 0
			Return ret;
			ret.ret = new IntegerLiteral(0);
			ret.emitFunctionScope(out);
		}


		out << "}\n";
	}
};



#endif