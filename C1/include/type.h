#ifndef COMPILER_TYPE_H
#define COMPILER_TYPE_H

#include <vector>
#include <string_view>
#include <variant>
#include <memory>
#include <functional>

#include "cppType.h"

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
	std::variant<int64_t, double, std::string_view> value;
};

constexpr PrimitiveType PRIMITIVE_TYPES[] = {
	{ PrimitiveType::UINT8, int64_t(0) },
	{ PrimitiveType::UINT16, int64_t(0) },
	{ PrimitiveType::UINT32, int64_t(0) },
	{ PrimitiveType::INT32, int64_t(0) },
	{ PrimitiveType::FLOAT, 0.0 },
	{ PrimitiveType::STRING, "" }
};

struct Expression;
struct FuncEmitter;

struct VariableDeclaration {
	std::string_view name;
	CppType* type;
	Expression* initializer;

	int valReg = 0;
	std::string getValReg() {
		std::string ret = "%";
		ret.append(name);
		ret.append(".");
		ret.append(std::to_string(valReg));
		return ret;
	}

	std::string getNextValReg() {
		valReg++;
		return getValReg();
	}
};

enum class DeclarationType {
	FunctionDef, // Function*
	FunctionDecl, // FunctionPrototype*

	TemplateDecl,
	TemplateInstantiation,
	TemplateSpecialization,

	NamespaceDecl, // Scope*
	NamespaceAlias, // Scope*

	AsmDecl,
	TypeAlias, // CppType*

	UsingDecl, // CppType*, imports a name from a different scope into this one
	UsingDirective, // Scope*, imports an entire namespace

	StaticAssert, // Expression*

	Simple, // CppType*, Expression*, initializes an identifier
};

struct Function;
class Scope;
struct CppType;
struct Expression;
struct FunctionPrototype;

struct Declaration {
	using Data = std::variant<Function*, FunctionPrototype*, Scope*, CppType*, Expression*, VariableDeclaration*>;

	std::string_view name;

	Data data;
};

#endif