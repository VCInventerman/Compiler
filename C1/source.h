#ifndef COMPILER_SOURCE_H
#define COMPILER_SOURCE_H

#include <string_view>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <sstream>

struct SourceOffset {
	int64_t line;
	int64_t col;
};

struct SourceSnippet {
	SourceOffset begin;
	SourceOffset end;
};

// Represents a string in a source code file
struct SourcePos {
	std::string_view filename;
	std::string_view file;

	const char* begin;
	const char* end;

	// Figures out at what line and offset a string view is placed
	SourceOffset findPtr(const char* where) const {
		int64_t line = 1;
		int64_t column = 1;
		for (const char* i = file.data(); i < file.data() + file.size(); i++) {
			if (i == file.data()) {
				return { line, column };
			}

			if (*i == '\n') {
				line++;
				column = 1;
			}
			else {
				column++;
			}
		}

		throw std::range_error(std::string("File pointer didn't point inside file: ") + std::string(file));
	}

	// Figures out at what line and offset a string view is placed, and where it ends
	SourceSnippet resolve() const {
		return { findPtr(begin), findPtr(end) };
	}

	operator std::string() const {
		SourceSnippet location = resolve();

		std::stringstream out;
		out << "in file " << file << ", line " << location.begin.line << ", column " << location.begin.col;
		return out.str();
	}

	std::string_view str() {
		return std::string_view(begin, end - begin);
	}
};

#endif // ifndef COMPILER_SOURCE_H