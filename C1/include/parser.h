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
#include "expression.h"
#include "function.h"

struct Parser {
	Scanner scanner;

	Token currentTok = { {}, TokenType::UNKNOWN };
	std::vector<Scope*> scopes;

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
		auto next = scanner.peek();
		auto str = next.first;

		std::cout << "Produce token: " << str << '\n';

		currentTok = {};

		if (str == "") {
			currentTok.type = TokenType::END_OF_FIELD;
			return;
		}

		currentTok.code.begin = str.data();
		currentTok.code.end = str.data() + str.size();

		auto operatorIndex = std::find(std::begin(OPERATOR_TOKENS), std::end(OPERATOR_TOKENS), str);
		if (operatorIndex != std::end(OPERATOR_TOKENS)) {
			currentTok.type = (TokenType)(operatorIndex - OPERATOR_TOKENS);
			scanner.readCursor = next.second;
			return;
		}

		if (std::all_of(str.cbegin(), str.cend(), [](char c) { return std::isdigit(c); })) {
			// A token consisting entirely of digits is an integer or the first half of a float
			auto val = scanFloatLiteral(str);
			currentTok.type = TokenType::PRIMITIVE;
			scanner.readCursor = next.second;
			return;
		}
		else {
			auto name = consumeName();

			currentTok.type = TokenType::IDENTIFIER;
			currentTok.defaultValue = name;
		}

		//todo: Hex
	}

	Expression* parseTerminalExpression() {
		// Creates an AstNode from the current token, assuming it is a value
		if (currentTok.type == TokenType::PRIMITIVE) {
			auto out = new IntegerLiteral(std::any_cast<int>(currentTok.defaultValue));
			return out;
		}
		else {
			std::cout << "Expected terminal token but got " << OPERATOR_TOKENS[int(currentTok.type)] << '\n';
			//auto out = AstNode::makeLeaf(currentTok);
			return nullptr;
		}
		//todo: var

		throw NULL;
	}

	Expression* parsePrefixExpression(Scope* scope) {
		switch (currentTok.type) {
		case TokenType::PLUS: {
			return new UnaryAdd(parseExpression(scope));
		}
		case TokenType::MINUS: {
			return new UnarySub(parseExpression(scope));
		}
		case TokenType::MULT: {

			break;
		}
		case TokenType::AMPERSAND: {

			break;
		}
		}

		return parseTerminalExpression();
	}

	Expression* parseFunctionCall(Scope* scope) {
		try {
			auto vScan = scanner.startVirtualScan();

			std::string_view name = consumeName();

			auto res = scope->lookup(name);

			if (Function** fn = std::get_if<Function*>(&res->data)) {
				auto call = new FunctionCall(*fn);

				call->arguments = consumeFunctionPassedParameters();

				scanToken();

				vScan.keep();
				return call;
			}
		}
		catch (...) {}

		return nullptr;
	}

	Expression* parseExpression(Scope* scope, int previousTokenPrecedence = 999) {
		if (Expression* call = parseFunctionCall(scope)) {
			return call;
		}

		scanToken();

		// Get the left expression
		Expression* left = parsePrefixExpression(scope);
		Expression* right;

		auto next = scanner.peek().first;
		if (next == ")" || next == ",'") {
			return left;
		}

		scanToken(); // Scan in the binary operator
		TokenType nodeType = currentTok.type;
		int precedence = OPERATOR_PRECEDENCE[(int)nodeType];

		if (currentTok.type == TokenType::END_OF_FIELD) {
			std::cout << "missing semicolon\n";
			throw NULL;
		}
		else if (currentTok.type == TokenType::SEMICOLON || currentTok.type == TokenType::RIGHT_PAREN || currentTok.type == TokenType::COMMA) {
			return left;
		}

		while (precedence <= previousTokenPrecedence) {
			right = parseExpression(scope, precedence);

			Token parent;
			parent.type = nodeType;
			left = makeBinaryExp(nodeType, left, right);
			//left = AstNode::makeParent(parent, std::move(left), std::move(right));

			nodeType = currentTok.type;
			if (nodeType == TokenType::END_OF_FIELD) {
				std::cout << "missing semicolon\n";
				throw NULL;
			}
			else if (nodeType == TokenType::SEMICOLON || nodeType == TokenType::RIGHT_PAREN || nodeType == TokenType::COMMA) {
				break;
			}

			auto next = scanner.peek().first;
			if (next == ")" || next == ",'") {
				return left;
			}
		}

		return left;
	}

	void matchToken(TokenType type) {
		scanToken();

		if (currentTok.type != type) {
			std::cout << "didnt find expected token type\n";
			throw NULL;
		}
	}

	std::string_view consumeName() {
		// Consume a sequence of :: and names
		// May not end with ::

		auto isScopeOp = [](std::string_view str) {
			return str == "::";
		};

		// Set to false so that a :: may precede
		bool lastWasScopeOp = false;

		scanner.readCursor = scanner.skipws(scanner.readCursor, scanner.code.data() + scanner.code.size());
		std::string_view name = { scanner.readCursor, 0 };

		while (*scanner.readCursor != ' ' && !isAnyOf(*scanner.readCursor, std::begin(OPERATOR_CHARACTERS), std::end(OPERATOR_CHARACTERS))) {
			auto [ str, itr ] = scanner.peek();

			if (isScopeOp(str)) {
				if (lastWasScopeOp) {
					throw NULL;
				}
				name = std::string_view(name.data(), name.size() + str.size());
				lastWasScopeOp = true;
				scanner.readCursor = itr;
			}
			else if (scanner.isValidIdentifier(str)) {
				if (!lastWasScopeOp && name != "") {
					throw NULL;
				}
				name = std::string_view(name.data(), name.size() + str.size());
				lastWasScopeOp = false;
				scanner.readCursor = itr;
			}
			else {
				if (lastWasScopeOp) {
					throw NULL;
				}

				return name;
			}
		}

		if (name.size() == 0) {
			throw NULL;
		}

		return name;
	}

	void parse(Scope* scope) {
		scopes.push_back(scope);

		while (true) {
			auto next = scanner.peek().first;

			if (next == "") {
				break;
			}
			else if (next == "}") {
				break;
			}
			else if (next == ";") {
				continue;
			}

			else if (parseDeclaration(scope)) { continue; }
			else if (parseFunction(scope)) { continue; }
			else if (parseReturn(scope)) { continue; }
			else if (parseStatementExpression(scope)) { continue; }

			else {
				scanToken();
				break;
			}
		}

		scopes.pop_back();
	}

	bool parseStatementExpression(Scope* scope) {
		auto virtualScanner = scanner.startVirtualScan();

		Expression* exp = parseExpression(scope);

		if (currentTok.type != TokenType::SEMICOLON) {
			throw NULL;
		}

		scope->addExpression(exp);
		virtualScanner.keep();
		return true;
	}

	bool parseReturn(Scope* scope) {
		try {
			auto virtualScanner = scanner.startVirtualScan();

			std::string_view retToken = consumeName();
			if (retToken != "return") { return false; }

			Return* retExp = new Return;

			retExp->ret = parseExpression(scope);

			scope->addExpression(retExp);
			virtualScanner.keep();
			return true;
		}
		catch (...) {
			return false;
		}
	}

	void parseClassDecl(std::string_view type) {
		bool defaultPrivateVisibility = type == "class";
		
		// started with "class" or "struct"
		// not parsing attributes

		auto name = consumeName();

		if (scanner.peek().first == "final") {
		
		}
		
		// Ignore inheritance
		while (scanner.peek().first != "{") {
			scanner.consume();
		}

		parseClassScope();
	}
	void parseClassScope() {

	}

	void parseEnumDecl() {
		bool isNamespace = false;

		// started with "enum"
		// may be class or struct to create a namespace
		auto firstTok = scanner.peek();

		if (firstTok.first == "class" || firstTok.first == "struct") {
			isNamespace = true;
			scanner.readCursor = firstTok.second;
		}

		// no attribute parsing for now

		if (scanner.peek().first != ":" && scanner.peek().first != "{") {
			auto name = consumeName();
		}

		if (scanner.peek().first != "{") {
			matchToken(TokenType::COLON);
			auto type = scanner.consume();
		}

		parseEnumScope();
	}
	void parseEnumScope() {
		matchToken(TokenType::LEFT_BRACE);

		matchToken(TokenType::RIGHT_BRACE);
		matchToken(TokenType::SEMICOLON);
	}

	void parseUnionDecl() {

	}
	void parseUnionScope() {

	}

	bool parseDeclaration(Scope* scope) {
		try {
			auto virtualScanner = scanner.startVirtualScan();

			Declaration decl;

			auto type = consumeName();
			CppType* cppType = strToType("int");

			auto name = consumeName();
			decl.name = name;

			auto next = scanner.consume();

			if (next == "=") {
				auto exp = parseExpression(scope);
			}
			else {
				decl.data = new VariableDeclaration{ name, cppType, new IntegerLiteral(0) };
			}

			matchToken(TokenType::SEMICOLON);


			virtualScanner.keep();
			return true;
		}
		catch (...) {}

		return false;
	}

	bool parseFunction(Scope* scope) {
		try {
			if (!scope->isFile() && !scope->isClass()) {
				return false;
			}

			auto virtualScanner = scanner.startVirtualScan();
			Function* fn = new Function(scope);

			//type
			auto type = consumeName();
			if (type.size() == 0) { throw NULL; }

			fn->decl.returnType = strToType(type);

			auto name = consumeName();
			consumeFunctionParameters(*fn);

			auto next = scanner.consume();

			fn->decl.name = name;

			if (next == "{") {
				fn->defined = true;
				parse(&fn->body);
			}

			scanToken();

			if (currentTok.code.str() != "}") {
				throw NULL;
			}

			virtualScanner.keep();
			scope->addFunction(fn);
			return true;
		}
		catch (...) {
		}

		return false;
	}

	std::vector<Expression*> consumeFunctionPassedParameters() {
		matchToken(TokenType::LEFT_PAREN);

		std::vector<Expression*> args;

		auto next = scanner.peek();
		while (next.first != ")") {
			args.push_back(parseExpression(scopes.back()));

			next = scanner.peek();
		}

		matchToken(TokenType::RIGHT_PAREN);

		return args;
	}

	void consumeFunctionParameters(Function& fn) {
		matchToken(TokenType::LEFT_PAREN);

		auto next = scanner.peek();
		while (next.first != ")") {
			scanner.readCursor = next.second;

			next = scanner.peek();
		}

		matchToken(TokenType::RIGHT_PAREN);
	}

	void parseMemberFunction(Scope* scope) {
		if (scope->isClass()) {

		}
	}

	// Parse a sequnce of keywords and type specifiers (names)
	void parseDeclSpecifierSeq(Scope* scope) {
		
	}

	bool isTypeSpecifier(std::string_view type) {
		// Simple, individual word level analysis

		bool valid = false;

		if (type == "auto" || 
			std::find(std::begin(SIMPLE_TYPE_SPECIFIERS_STR), std::end(SIMPLE_TYPE_SPECIFIERS_STR), type) != std::end(SIMPLE_TYPE_SPECIFIERS_STR)) {
			return true;
		}
	}

	bool parseTypeSpecifierSeq(Scope* scope) {
		std::vector<std::string_view> specifiers;
		CppType type;

		auto first = scanner.peek();
		while (isTypeSpecifier(first.first)) {
			scanner.readCursor = first.second;
			specifiers.push_back(first.first);


		}

		if (specifiers.size() == 0) {
			return false;
		}

		// Ending rules
		// Specifiers that may only be declared once
		switch (count(specifiers, "const")) {
		case 0: type.isConst = false; break;
		case 1: type.isConst = true; break;
		default: return false;
		}

		switch (count(specifiers, "volatile")) {
		case 0: type.isVolatile = false; break;
		case 1: type.isVolatile = true; break;
		default: return false;
		}

		// signed or unsigned can be combined with char, long, short, or int.
		if (has(specifiers, "signed")) {
			if (has(specifiers, "unsigned")) { return false; }

			auto list = { "char", "long", "short", "int" };
			if (!hasAnyOf(specifiers, list)) { return false; }
		}
	}
};

#endif