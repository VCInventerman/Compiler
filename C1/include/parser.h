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
#include "type.h"
#include "scanner.h"
#include "expression.h"
#include "function.h"

struct Parser {
	Scanner scanner;

	Token currentTok;
	std::vector<Scope*> scopes;

	Parser(std::string_view _code, std::string_view _file) : scanner(_code, _file) {}

	void scanToken() {
		auto next = scanner.peek();
		auto tok = next.first;
		auto str = tok.str;

		currentTok = {};

		currentTok.origCode.begin = str.data();
		currentTok.origCode.end = str.data() + str.size();

		if (str == "") {
			currentTok.type = TokenType::END_OF_FIELD;
			return;
		}


		// Attempt to map str to an operator
		auto operatorIndex = FIND_ARRAY_SUBMEMBER(OPERATOR_TRAITS, str, str);
		if (operatorIndex != std::end(OPERATOR_TRAITS)) {
			currentTok.str = next.first.str;
			currentTok.op = (Operator)(operatorIndex - OPERATOR_TRAITS);
			scanner.readCursor = next.second;
			return;
		}

		// A token consisting entirely of digits is an integer or the first half of a float
		if (std::all_of(str.cbegin(), str.cend(), [](char c) { return std::isdigit(c); })) {
			const char* r = str.data();
			auto val = LiteralParser::parseLiteral(r, str.data() + str.size());
			currentTok.type = TokenType::INTEGER_LITERAL;
			currentTok.value = val;
			scanner.readCursor = next.second;
			return;
		}
		else {
			// Might be a valid name
			auto name = consumeName();

			currentTok.str = name;
			currentTok.type = TokenType::IDENTIFIER;
			currentTok.value = (std::string)name;
		}

		//todo: Hex
	}

	Expression* parseLeftExpression(Scope* scope) {
		if (currentTok.type == TokenType::INTEGER_LITERAL) {
			return new IntegerLiteral((int)*std::get_if<int64_t>(&currentTok.value));
		}
		if (Expression* exp = parseVariableRef(scope)) {
			return exp;
		}
		else if (Expression* exp = parseFunctionCall(scope)) {
			return exp;
		}

		else if (currentTok.str == "+") {
			return new UnaryAdd(parseExpression(scope));
		}
		else if (currentTok.str == "-") {
			return new UnarySub(parseExpression(scope));
		}

		else if (currentTok.str == "*") {

		}
		else if (currentTok.str == "&") {

		}
		
		else if (currentTok.str == "!") {

		}
		else if (currentTok.str == "~") {

		}

		else if (currentTok.str == "++") {

		}
		else if (currentTok.str == "--") {

		}

		return nullptr;
	}

	Expression* parseVariableRef(Scope* scope) {
		try {
			auto vScan = scanner.startVirtualScan();

			auto res = scope->lookup(currentTok.str);

			if (!res) { return nullptr; }

			if (VariableDeclaration** var = std::get_if<VariableDeclaration*>(&res->data)) {
				auto ref = new VariableRef(*var);

				vScan.keep();
				return ref;
			}
		}
		catch (...) {}

		return nullptr;
	}

	Expression* parseFunctionCall(Scope* scope) {
		try {
			auto vScan = scanner.startVirtualScan();

			if (currentTok.type != TokenType::IDENTIFIER) {
				return nullptr;
			}

			std::string_view name = currentTok.str;


			//std::string_view name = consumeName();

			auto res = scope->lookup(name);

			if (!res) {
				return nullptr;
			}

			if (Function** fn = std::get_if<Function*>(&res->data)) {
				auto call = new FunctionCall(*fn);

				call->arguments = consumeFunctionPassedParameters();

				vScan.keep();
				return call;
			}
		}
		catch (...) {}

		return nullptr;
	}

	Expression* parseExpression(Scope* scope, int previousTokenPrecedence = 999) {
		auto next = scanner.peek().first;
		if (next.str == ")" || next.str == "") {
			return nullptr;
		}
		else if (next.str == ";") {
			return new EmptyExpression;
		}

		scanToken();

		Expression* left = parseLeftExpression(scope);

		next = scanner.peek().first;
		if (next.str == ")" || next.str == ";" || next.str == "" || !left) {
			return left;
		}

		// Scan in a binary or unary postfix operator
		scanToken();

		if (currentTok.str == ";" || currentTok.str == "" || currentTok.str == ")") {
			return left;
		}
		if (currentTok.str == "++") {


		}
		else if (currentTok.str == "--") {

		}

		int precedence = OPERATOR_TRAITS[(int)currentTok.op].precedence;
		while (precedence <= previousTokenPrecedence) {
			Token parent;
			parent.str = currentTok.str;

			Expression* right = parseExpression(scope, precedence);

			if (right == nullptr) {
				return left;
			}

			left = makeBinaryExp(parent.str, left, right);

			next = scanner.peek().first;
			if (next.str == ")" || next.str == ";" || next.str == "") {
				return left;
			}

			scanToken();

			TokenType nodeType = currentTok.type;
			if (nodeType == TokenType::END_OF_FIELD) {
				std::cout << "missing semicolon\n";
				throw NULL;
			}
			else if (currentTok.str == ";" || currentTok.str == ")") {
				break;
			}
		}

		return left;
	}


	void matchCurrentToken(std::string_view tok) {
		if (currentTok.str != tok) {
			std::cout << "didnt find expected token type\n";
			throw NULL;
		}
	}

	void matchToken(std::string_view tok) {
		scanToken();

		matchCurrentToken(tok);
	}

	std::string_view consumeName() {
		// Consume a sequence of :: and names
		// May not end with ::

		auto isScopeOp = [](std::string_view str) {
			return str == "::";
		};

		// Set to false so that a :: may precede
		bool lastWasScopeOp = false;

		scanner.readCursor = scanner.skipws();
		std::string_view name = { scanner.readCursor, 0 };

		while (true) {
			if (*scanner.readCursor == ' ') {
				if (name.size() == 0 || name[0] == ' ') {
					throw NULL;
				}
				return name;
			}

			auto [ tok, itr ] = scanner.peek();
			auto str = tok.str;

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

				if (name.size() == 0 || name[0] == ' ') {
					throw NULL;
				}

				return name;
			}
		}

		if (name.size() == 0 || name[0] == ' ') {
			throw NULL;
		}

		return name;
	}

	void parse(Scope* scope) {
		scopes.push_back(scope);

		while (true) {
			auto next = scanner.peek().first.str;

			if (next == "" || next == "}") {
				// End scope
				break;
			}
			else if (next == ";") {
				continue;
			}
			else if (next == "{") {
				// Enter a block scope
				//todo: this
			}

			else if (parseDeclaration(scope)) { continue; }
			else if (parseFunction(scope)) { continue; }
			else if (parseReturn(scope)) { continue; }
			else if (parseStatementExpression(scope)) { continue; }

			else {
				// Give up and consume this unknown token
				std::cout << "Unknown token " << next << '\n';
				scanToken();
				break;
			}
		}

		scopes.pop_back();
	}

	bool parseStatementExpression(Scope* scope) {
		auto virtualScanner = scanner.startVirtualScan();

		Expression* exp = parseExpression(scope);

		matchToken(";");

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
		bool defaultPrivateVisibility = (type == "class");
		
		// started with "class" or "struct"
		// not parsing attributes

		auto name = consumeName();

		if (scanner.peek().first.str == "final") {
		
		}
		
		// Ignore inheritance
		while (scanner.peek().first.str != "{") {
			scanner.consume();
		}
	}

	void parseEnumDecl() {
		bool isNamespace = false;

		// started with "enum"
		// may be class or struct to create a namespace
		auto firstTok = scanner.peek();

		if (firstTok.first.str == "class" || firstTok.first.str == "struct") {
			isNamespace = true;
			scanner.readCursor = firstTok.second;
		}

		// no attribute parsing for now

		if (scanner.peek().first.str != ":" && scanner.peek().first.str != "{") {
			auto name = consumeName();
		}

		if (scanner.peek().first.str != "{") {
			matchToken(":");
			auto type = scanner.consume();
		}

	}

	void parseUnionDecl() {

	}

	bool parseDeclaration(Scope* scope) {
		try {
			auto virtualScanner = scanner.startVirtualScan();

			Declaration decl;

			auto type = consumeName();
			CppType* cppType = strToType(type);

			if (cppType == nullptr) {
				std::cout << "Invalid type " << type << "\n";
			}

			auto name = consumeName();
			decl.name = name;

			auto next = scanner.consume();

			VariableDeclaration* varDecl = new VariableDeclaration{ name, cppType, new IntegerLiteral(0) };
			decl.data = varDecl;

			if (next.str == "=") {
				auto exp = parseExpression(scope);
				varDecl->initializer = exp;
			}

			VariableDeclExp* exp = new VariableDeclExp(varDecl);

			matchToken(";");

			scope->addDeclaration(decl);
			scope->addExpression(exp);

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

			auto type = consumeName();
			fn->decl.returnType = strToType(type);

			auto name = consumeName();
			consumeFunctionParameters(*fn);

			auto next = scanner.consume();

			fn->decl.name = name;

			if (next.str == "{") {
				fn->defined = true;
				parse(&fn->body);

				matchToken("}");
			}
			else if (next.str == ";") {
				// Just a declaration
			}
			else {
				throw NULL;
			}

			virtualScanner.keep();
			scope->addFunction(fn);
			return true;
		}
		catch (...) {}

		return false;
	}

	std::vector<Expression*> consumeFunctionPassedParameters() {
		matchToken("(");

		std::vector<Expression*> args;

		auto next = scanner.peek();
		while (next.first.str != ")" && next.first.str != "") {
			args.push_back(parseExpression(scopes.back()));

			next = scanner.peek();
		}

		matchToken(")");

		return args;
	}

	void consumeFunctionParameters(Function& fn) {
		matchToken("(");

		auto next = scanner.peek();
		while (next.first.str != ")") {
			scanner.readCursor = next.second;

			next = scanner.peek();
		}

		matchToken(")");
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
		while (isTypeSpecifier(first.first.str)) {
			scanner.readCursor = first.second;
			specifiers.push_back(first.first.str);


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