#ifndef COMPILER_TYPE_H
#define COMPILER_TYPE_H

#include <vector>
#include <string_view>
#include <variant>
#include <memory>

#include "ast.h"

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
	std::weak_ptr<void> initializer;
};

struct Identifier {
	std::string_view name;
};

struct Scope {
	Scope* parent;

	std::vector<Declaration> declarations;
	std::vector<Identifier> identifiers;
};

struct Function {
	// prototype?

	Scope definition;
	std::vector<std::unique_ptr<AstNode>> statements;
};

#endif