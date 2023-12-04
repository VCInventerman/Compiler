#ifndef COMPILER_CPPTYPE_H
#define COMPILER_CPPTYPE_H

#include <string>
#include <regex>

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
		_coreName = std::string(str_);

		const std::regex nameRegex("[_a-zA-Z][_a-zA-Z0-9]*");
		std::smatch nameMatch;

		if (std::regex_search(_coreName, nameMatch, nameRegex)) {
			// The first sub_match is the whole string; the next
			// sub_match is the first parenthesized expression.
			if (nameMatch.size() == 1)
			{
				_coreName = nameMatch[0].str();
			}
		}

		_pointerLayers = (int)std::count(str_.begin(), str_.end(), '*');
		_pointerLayerIsConst.resize(_pointerLayers, false);

		make();
	}

	void make() {
		makeName();
		makeLlvmName();
	}

	void makeLlvmName() {
		if (_coreName == "void") {
			_llvmName = "void";
			_width = 0;
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
		else if (_coreName == "_condition") {
			_llvmName = "i1";
			_width = 0;
			_isInteger = true;
		}
		else if (_coreName == "nullptr_t") {
			_llvmName = "void*";
			_width = currentDataModel->pointerWidth;
			_isInteger = true;
		}
		else {
			throw NULL;
		}

		if (_pointerLayers) {
			_width = currentDataModel->pointerWidth;
			_isInteger = true;
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

	return type;
}

#endif // ifndef COMPILER_CPPTYPE_H