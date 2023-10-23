#ifndef COMPILER_LLVMTYPE_H
#define COMPILER_LLVMTYPE_H


struct LlvmType {
	virtual std::string name() = 0;
};

struct LlvmTypeVoid : public LlvmType {
	int size() {
		return 0;
	}

	std::string name() {
		return "void";
	}
};

struct LlvmTypeFunction : public LlvmType {
	LlvmType* returns;
	std::vector<LlvmType*> args;

	std::string name() {
		std::string out = returns->name();
		out += " (";

		for (auto& arg : args) {
			out += arg->name();
			out += ", ";
		}

		out += ")";
		return out;
	}
};

// First class types can be produced by instructions and stored in registers
struct LlvmTypeInt : public LlvmType {
	int width;

	std::string name() {
		return "i" + std::to_string(width);
	}
};

struct LlvmTypeFloat : public LlvmType {
	enum FLOAT_TYPES {
		HALF,
		BFLOAT,
		FLOAT,
		DOUBLE,
		FP128,
		X86_FP80,
		PPC_FP128
	};

	static constexpr std::string_view FLOAT_TYPES_STR[] = {
		"half",
		"bfloat",
		"double",
		"fp128",
		"x86_fp80",
		"ppc_fp128"
	};


	FLOAT_TYPES type;

	std::string name() {
		return std::string(FLOAT_TYPES_STR[(int)type]);
	}
};

// LLVM uses opaque pointers as of LLVM 15, but my clang is too old for that
struct LlvmTypePtr : public LlvmType {
	LlvmType* destType;

	std::string name() {
		return destType->name() + "*";
	}
};

struct LlvmTypeVec : public LlvmType {
	int size; // Element count
	LlvmType* dataType;

	std::string name() {
		std::string out = "<";
		out += std::to_string(size);
		out += " x ";
		out += dataType->name();
		out += ">";
		return out;
	}
};

struct LlvmTypeLabel : public LlvmType {
	std::string _name;

	std::string name() {
		return _name;
	}
};

struct LlvmTypeToken : public LlvmType {
	std::string _name;

	std::string name() {
		return _name;
	}
};

struct LlvmTypeMetadata : public LlvmType {
	std::string _name;

	std::string name() {
		return _name;
	}
};

struct LlvmTypeArray : public LlvmType {
	int size; // Element count
	LlvmType* dataType;

	std::string name() {
		std::string out = "[";
		out += std::to_string(size);
		out += " x ";
		out += dataType->name();
		out += "]";
		return out;
	}
};

struct LlvmTypeLiteralStruct : public LlvmType {
	std::vector<LlvmType*> members;
	bool packed = false;

	std::string name() {
		std::string out = "type { ";

		for (auto& member : members) {
			out += member->name();
			out += ", ";
		}

		out += " }";
		return out;
	}

	bool equals(LlvmType* rhs) {
		return false;
	}
};

struct LlvmTypeIdentifiedStruct : public LlvmType {
	std::string prettyName;
	std::string codeName;
	LlvmTypeLiteralStruct type;

	void init() {
		codeName = "%struct.";
		codeName += prettyName;
	}

	std::string declare() {
		std::string out = codeName;
		out += " = ";
		out += type.name();
		return out;
	}

	std::string name() {
		return codeName;
	}
};

struct LlvmTypeOpaqueStruct : public LlvmType {
	std::string name() {
		return "type opaque";
	}
};


// Constant types
struct LlvmTypeConstantBool : public LlvmType {
	bool val;

	std::string name() {
		return val ? "true" : "false";
	}
};

struct LlvmTypeConstantInt : public LlvmType {
	std::string val;

	std::string name() {
		return val;
	}
};

struct LlvmTypeConstantFloat : public LlvmType {
	std::string val;

	std::string name() {
		return val;
	}
};

struct LlvmTypeConstantNullPtr : public LlvmType {
	std::string name() {
		return "null";
	}
};

struct LlvmTypeConstantToken : public LlvmType {
	std::string name() {
		return "none";
	}
};

#endif // ifndef COMPILER_LLVMTYPE_H