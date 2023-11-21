#ifndef COMPILER_CPPTYPE_H
#define COMPILER_CPPTYPE_H

#include <string>

#include "token.h"

struct CppType {
	// Applies to class, union, enum, typedef, and type alias
	std::string _coreName; // Underlying type like int or a class

	// Derived information
	int _width;
	std::string _name;
	std::string _llvmName;

	bool _isSigned = false;
	bool _isInteger = true;
	bool _isConst = false;
	bool _isVolatile = false;

	int _pointerLayers = 0;
	std::vector<bool> _pointerLayerIsConst;

	bool _isReference = false; // Must be top layer

	CppType() {}

	CppType(std::string_view str_) {
		_coreName = str_;
	}

	void makeLlvmName() {
		if (_coreName == "void") {
			_llvmName = "void";
			_isInteger = false;
		}
		else if (_coreName == "bool") {
			_llvmName = buildStr("i", currentDataModel->charWidth);
			_width = currentDataModel->charWidth;
			_isInteger = true;
		}
		else if (_coreName == "char") {
			_llvmName = buildStr("i", currentDataModel->charWidth);
			_width = currentDataModel->charWidth;
			_isInteger = true;
		}
		else if (_coreName == "int") {
			_llvmName = buildStr("i", currentDataModel->intWidth);
			_width = currentDataModel->intWidth;
			_isInteger = true;
		}
		else if (_coreName == "long") {
			_llvmName = buildStr("i", currentDataModel->longWidth);
			_width = currentDataModel->longWidth;
			_isInteger = true;
		}
		else if (_coreName == "long long") {
			_llvmName = buildStr("i", currentDataModel->longLongWidth);
			_width = currentDataModel->longLongWidth;
			_isInteger = true;
		}
		else if (_coreName == "float") {
			_llvmName = "f32";
			_width = 32;
			_isInteger = false;
		}
		else if (_coreName == "double") {
			_llvmName = "f64";
			_width = 64;
			_isInteger = false;
		}
		else if (_coreName == "long double") {
			_llvmName = buildStr("i", currentDataModel->longDoubleWidth);
			_width = currentDataModel->longDoubleWidth;
			_isInteger = false;
		}
		else {
			throw NULL;
		}

		for (int i = 0; i < _pointerLayers; i++) {
			_llvmName += "*";
		}

		if (_isReference) {
			_llvmName += "*";
		}
	}

	void makeName() {
		_name = "";

		if (_isConst) {
			_name += "const ";
		}
		if (_isVolatile) {
			_name += "volatile ";
		}

		_name += _coreName;

		for (int i = 0; i < _pointerLayers; i++) {
			_name += " *";
			if (_pointerLayerIsConst[i]) {
				_name += "const ";
			}
		}

		if (_isReference) {
			_name += "& ";
		}
	}

	std::string_view getLlvmName() {
		return _llvmName;
	}

	std::string_view getName() {
		return _name;
	}

	int width() {
		return _width;
	}

	bool isSigned() {
		return _isSigned;
	}

	bool isInteger() {
		return _isInteger;
	}

	bool operator==(const CppType& rhs) const noexcept {
		return this->_name == rhs._name;
	}
	bool operator!=(const CppType& rhs) const noexcept {
		return !(*this == rhs);
	}
};

inline CppType* strToType(std::string_view str) {
	auto type = new CppType(str);
	type->makeName();
	type->makeLlvmName();

	return type;
}

#endif // ifndef COMPILER_CPPTYPE_H