#ifndef COMPILER_TYPE_H
#define COMPILER_TYPE_H

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
	{ PrimitiveType::UINT8, 0 },
	{ PrimitiveType::UINT16, 0 },
	{ PrimitiveType::UINT32, 0 },
	{ PrimitiveType::INT32, 0 },
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

struct Scope {
	std::weak_ptr<Scope> parent;

	std::vector<Declaration> declarations;
};

#endif