#ifndef COMPILER_CPPTYPE_H
#define COMPILER_CPPTYPE_H

#include <string>

struct CppType {
	// Applies to class, union, enum, typedef, and type alias
	std::string name;
	std::string llvmName;

	bool isConst = false;
	bool isVolatile = false;

	bool isFundamental;
};

struct CppTypeVoid {
	static CppType* make() { return new CppType{ "void", "void" }; }
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
};

inline CppType* strToType(std::string_view str) {
	if (str == "void") {
		return new CppType{ "void", "void" };
	}
	else if (str == "bool") {
		return new CppType{ "bool", "i1" };
	}
	else if (str == "int") {
		return new CppType{ "int", "i32" };
	}
	else if (str == "float") {
		return new CppType{ "float", "f32" };
	}

	return nullptr;
}

#endif // ifndef COMPILER_CPPTYPE_H