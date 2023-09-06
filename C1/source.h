#ifndef COMPILER_SOURCE_H
#define COMPILER_SOURCE_H

#include <string_view>
#include <stdexcept>
#include <memory>
#include <iostream>

// Represents a string in a source code file
struct SourcePos {
	std::string_view filename;
	uint64_t line;
	uint64_t offset;

	const char* begin;
	const char* end;
};

// Figures out at what line and offset a string view is placed
SourcePos resolveSourcePos(std::string_view filename, std::string_view file, std::string_view target) {
	SourcePos ptr = { filename, 0, 0, target.data(), target.data() + target.size() };

	uint64_t line = 1;
	uint64_t offset = 1;
	for (const char* i = file.data(); i < file.data() + file.size(); i++) {
		if (i == target.data()) {
			ptr.line = line;
			ptr.offset = offset;
			return ptr;
		}

		if (*i == '\n') {
			line++;
			offset = 1;
		}
		else {
			offset++;
		}
	}

	if (target != "") {
		throw std::range_error("File pointer didn't point inside file!");
	}
	else {
		return { filename, 0, 0, file.data(), file.data() + 1 };
	}
}

#endif