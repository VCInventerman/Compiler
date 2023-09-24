#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include <array>
#include <string_view>
#include <functional>
#include <algorithm>
#include <charconv>
#include <string>

#include "util.h"
#include "token.h"
#include "ast.h"
#include "type.h"
#include "scanner.h"

struct Parser {
	Scanner scanner;

	Token currentTok = { {}, TokenType::UNKNOWN };

	Parser(std::string_view _code, std::string_view _file) : scanner(_code, _file) {}

	bool scanFloatLiteral(std::string_view wholeStr) {
		auto [period, periodR] = scanner.peek();
		auto [decimal, decimalR] = scanner.peekAt(periodR);

		if (period == "." && std::all_of(decimal.cbegin(), decimal.cend(), [](char c) { return std::isdigit(c); })) {
			double num = 0.0;
			
			// GCC doesn't support from_chars yet
			/*auto res = std::from_chars(wholeStr.data(), decimal.data() + decimal.size(), num);

			if (res.ec == std::errc{}) {
				r = r2;
				currentTok.defaultValue = num;
				return true;
			}*/
			std::string tempBuf = std::string(wholeStr.data(), (decimal.data() + decimal.size()) - wholeStr.data());
			num = std::stod(tempBuf);

			scanner.readCursor = periodR;
			currentTok.defaultValue = num;
			return true;
		}
		else {
			int num = 0;
			auto res = std::from_chars(wholeStr.data(), wholeStr.data() + wholeStr.size(), num);

			if (res.ec == std::errc{}) {
				currentTok.defaultValue = num;
				return true;
			}
		}

		return false;
	}

	void scanToken() {
		auto str = scanner.consume();

		std::cout << "Produce token: " << str << '\n';

		currentTok = {};

		if (str == "") {
			currentTok.type = TokenType::END_OF_FIELD;
			return;
		}

		currentTok.code.begin = str.data();
		currentTok.code.begin = str.data() + str.size();

		auto operatorIndex = std::find(std::begin(OPERATOR_TOKENS), std::end(OPERATOR_TOKENS), str);
		if (operatorIndex != std::end(OPERATOR_TOKENS)) {
			currentTok.type = (TokenType)(operatorIndex - OPERATOR_TOKENS);
			return;
		}

		if (std::all_of(str.cbegin(), str.cend(), [](char c) { return std::isdigit(c); })) {
			// A token consisting entirely of digits is an integer or the first half of a float
			auto val = scanFloatLiteral(str);
			currentTok.type = TokenType::PRIMITIVE;
			return;
		}

		//todo: Hex
	}

	std::unique_ptr<AstNode> parseTerminalNode() {
		// Creates an AstNode from the current token, assuming it is a value
		if (currentTok.type == TokenType::PRIMITIVE) {
			auto out = AstNode::makeLeaf(currentTok);
			scanToken();
			return out;
		}
		else {
			//std::cout << "Expected terminal token but got " << OPERATOR_TOKENS[int(currentTok.type)] << '\n';
			auto out = AstNode::makeLeaf(currentTok);
			scanToken();
			return out;
		}

		throw NULL;
	}

	std::unique_ptr<AstNode> parseBinaryExpression(int previousTokenPrecedence) {
		std::unique_ptr<AstNode> left = parseTerminalNode();
		std::unique_ptr<AstNode> right;
		TokenType nodeType = currentTok.type;

		if (currentTok.type == TokenType::SEMICOLON) {
			return left;
		}
		else if (currentTok.type == TokenType::END_OF_FIELD) {
			std::cout << "missing semicolon\n";
			throw NULL;
		}

		while (OPERATOR_PRECEDENCE[(int)nodeType] <= previousTokenPrecedence) {
			scanToken();

			int precedence = OPERATOR_PRECEDENCE[(int)nodeType];
			right = parseBinaryExpression(precedence);

			Token parent;
			parent.type = nodeType;
			left = AstNode::makeParent(parent, std::move(left), std::move(right));

			nodeType = currentTok.type;
			if (nodeType == TokenType::SEMICOLON) {
				break;
			}
			else if (nodeType == TokenType::END_OF_FIELD) {
				std::cout << "missing semicolon\n";
				throw NULL;
			}
		}

		return left;
	}

	void matchToken(TokenType type) {
		if (currentTok.type != type) {
			std::cout << "didnt find expected token type\n";
			throw NULL;
		}

		scanToken();
	}

	Function parse() {
		scanToken();

		// Single global fuction
		Function func;

		while (currentTok.type != TokenType::END_OF_FIELD) {
			matchToken(TokenType::PRINT);

			auto statement = parseBinaryExpression(999);

			matchToken(TokenType::SEMICOLON);

			func.statements.push_back(std::move(statement));
		}

		return func;
	}
};

#endif