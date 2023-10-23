#ifndef COMPILER_PREPROCESSOR_H
#define COMPILER_PREPROCESSOR_H

#include <string>
#include <string_view>

#include "scanner.h"

struct ScanToken {
	SourcePos orig;
	std::string_view text;
};

struct Preprocessor {
	std::string_view code;
	std::string out;

	Scanner scanner;

	std::vector<ScanToken> tokens;

	Preprocessor(std::string_view _code, std::string_view _file) : scanner(_code, _file) {}

	void process() {

	}
};

#endif