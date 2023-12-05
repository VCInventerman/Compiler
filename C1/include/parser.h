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
#include "flowControl.h"

struct Parser {
	Scanner scanner;

	Token currentTok;
	std::vector<Scope*> scopes;

	// If a function sets this flag, it made it too far into parsing before reaching an error, the code is wrong
	bool forceFail = false;

	Parser(std::string_view _code, std::string_view _file) : scanner(_code, _file) {}

	void parse(Scope* scope, bool captureSingleStatement = false) {
		scopes.push_back(scope);

		while (true) {
			forceFail = false;
			auto next = scanner.peek().first.str;

			if (next == "" || next == "}") {
				// End scope
				break;
			}
			else if (next == ";") {
				scanner.consume();
				continue;
			}
			else if (next == "{") {
				// Enter a block scope
				Scope* nextScope = new Scope("child", Scope::Type::FUNCTION, scope);

				matchToken("{");

				parse(nextScope);

				matchToken("}");

				scope->addChildScope(nextScope);
				scope->addExpression(new ChildScope(nextScope));
			}

			else if (parseDeclaration(scope)) { continue; }
			else if (parseFunction(scope)) { continue; }
			else if (parseReturn(scope)) { continue; }
			else if (parseIfStatement(scope)) { continue; }
			else if (parseStatementExpression(scope)) { continue; }

			else {
				if (forceFail) {
					std::cout << "Compile failed.\n";
					std::exit(-1);
				}

				// Give up and consume this unknown token
				std::cout << "Unknown token " << next << '\n';
				scanToken();
				break;
			}

			if (captureSingleStatement) {
				break;
			}
		}

		scopes.pop_back();
	}

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

		// Try to see if its an integer
		const char* r = str.data();
		auto val = LiteralParser::parseLiteral(r, str.data() + str.size());
		if (!std::get_if<LiteralContainerEmpty>(&val)) {
			currentTok.type = TokenType::INTEGER_LITERAL;
			currentTok.value = val;
			scanner.readCursor = next.second;
			return;
		}
		else {
			// Might be a valid name
			auto name = consumeName();

			if (name == "true" || name == "false") {
				currentTok.type = TokenType::BOOL_LITERAL;
				currentTok.value = int64_t(name == "true");
				scanner.readCursor = next.second;
				return;
			}

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
		else if (currentTok.type == TokenType::BOOL_LITERAL) {
			return new IntegerLiteral((int)*std::get_if<int64_t>(&currentTok.value));
		}
		else if (Expression* exp = parseCast(scope)) {
			return exp;
		}
		else if (Expression* exp = parseVariableRef(scope)) {
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
			Expression* exp = parseExpression(scope, OPERATOR_TRAITS[(int)Operator::DEREFERENCE].precedence);
			if (!exp) {
				return nullptr;
			}
			return new Dereference(exp);
		}
		else if (currentTok.str == "&") {

		}
		
		else if (currentTok.str == "!") {

		}
		else if (currentTok.str == "~") {
			return new BitwiseNot(parseExpression(scope));
		}

		else if (currentTok.str == "++") {

		}
		else if (currentTok.str == "--") {

		}

		else if (currentTok.str == "(") {
			Expression* exp = parseExpression(scope);
			matchToken(")");
			return exp;
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

			if (name == "alloca" || name == "__builtin_alloca") {
				matchToken("(");
				Expression* size = parseExpression(scope);
				matchToken(")");

				auto call = new StackAlloc(size);

				vScan.keep();
				return call;
			}
			else if (name == "sizeof") {
				matchToken("(");
				Expression* size = parseExpression(scope);
				matchToken(")");

				Expression* exp = new IntegerLiteral(size->getResultType()->width() / 8);

				vScan.keep();
				return exp;
			}
			
			if (!res) {
				return nullptr;
			}

			if (Function** fn = std::get_if<Function*>(&res->data)) {
				auto call = new FunctionCall(*fn);

				call->arguments = consumeFunctionPassedParameters(*fn);

				vScan.keep();
				return call;
			}
		}
		catch (...) {}

		return nullptr;
	}

	Expression* parseCast(Scope* scope) {
		try {
			auto vScan = scanner.startVirtualScan();

			if (currentTok.str == "(") {
				// (type)exp

				auto typeStr = consumeType();
				auto type = strToType(typeStr);

				matchToken(")");

				auto exp = parseExpression(scope);

				vScan.keep();
				return new Cast(exp, type);
			}
			else {
				// type(exp)

				auto typeStr = currentTok.str;
				auto type = strToType(typeStr);

				auto exp = parseExpression(scope);

				vScan.keep();
				return new Cast(exp, type);
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
		if ((next.str == ")" || next.str == ";") && left) {
			return left;
		}
		else if (next.str == "" || !left) {
			return nullptr;
		}
		
		auto virt = scanner.startVirtualScan();

		// Scan in a binary or unary postfix operator
		scanToken();

		if (OPERATOR_TRAITS[(int)currentTok.op].precedence > previousTokenPrecedence) {
			return left;
		}

		if (currentTok.str == ";" || currentTok.str == "" || currentTok.str == ")" || currentTok.str == ",") {
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
				virt.keep();
				return left;
			}

			

			/*scanToken();

			TokenType nodeType = currentTok.type;
			if (nodeType == TokenType::END_OF_FIELD) {
				std::cout << "missing semicolon\n";
				throw NULL;
			}
			else if (currentTok.str == ";" || currentTok.str == ")") {
				break;
			}*/
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

	std::string_view consumeType() {
		auto name = consumeName();

		while (*scanner.readCursor == '*') {
			scanner.readCursor++;
			name = std::string_view(name.data(), scanner.readCursor - name.data());
		}

		return name;
	}

	bool parseStatementExpression(Scope* scope) {
		try {
			auto virtualScanner = scanner.startVirtualScan();

			Expression* exp = parseExpression(scope);

			matchToken(";");

			scope->addExpression(exp);
			virtualScanner.keep();
			return true;
		}
		catch (...) {}

		return false;
	}

	bool parseIfStatement(Scope* scope) {
		try {
			auto virtualScanner = scanner.startVirtualScan();

			matchToken("if");

			forceFail = true;

			matchToken("(");

			Expression* exp = parseExpression(scope);

			matchToken(")");

			IfStatement* statement = new IfStatement(exp, scope);

			parse(&statement->trueBody, true);
			statement->hasTrueBranch = true;

			if (scanner.peek().first.str == "else") {
				scanner.consume();
				statement->hasFalseBranch = true;

				parse(&statement->falseBody, true);
			}

			scope->addExpression(statement);
			virtualScanner.keep();
			return true;
		}
		catch (...) {}

		return false;
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

			auto type = consumeType();
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

			if (scanner.peek().first.str == "__export") {
				fn->_export = true;
			}

			auto type = consumeName();
			fn->decl.returnType = strToType(type);

			auto name = consumeName();
			consumeFunctionParameters(*fn);

			auto next = scanner.consume();

			fn->decl.name = name;

			if (next.str == "{") {
				forceFail = true;
				fn->defined = true;
				parse(&fn->body);
				forceFail = true;

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

	std::vector<Expression*> consumeFunctionPassedParameters(Function* fn) {
		matchToken("(");

		std::vector<Expression*> args;

		auto next = scanner.peek();
		while (next.first.str != ")" && next.first.str != "") {
			// Parse the argument
			Expression* arg = parseExpression(scopes.back());

			if (arg == nullptr) {
				// Failed to parse the argument
				throw NULL;
			}

			if (fn->decl.arguments.size() <= args.size()) {
				// Found more arguments than function supports
				throw NULL;
			}

			// Get which argument of the function this entry corresponds with
			FunctionArgument* slot = fn->decl.arguments[args.size()];

			// Insert an intermediary cast expression if they don't match
			if (*arg->getResultType() != *(slot->type)) {
				args.push_back(new Cast(arg, slot->type));
			}
			else {
				args.push_back(arg);
			}

			next = scanner.peek();
		}

		matchToken(")");

		return args;
	}

	void consumeFunctionParameters(Function& fn) {
		matchToken("(");

		auto next = scanner.peek();

		if (next.first.str == ")") {
			scanner.readCursor = next.second;
			return;
		}

		while (next.first.str != ")") {
			std::vector<std::string_view> toks;

			while (next.first.str != ",") {
				if (next.first.str == ")") {
					break;
				}

				toks.push_back(next.first.str);
				scanner.readCursor = next.second;
				next = scanner.peek();
			}

			if (next.first.str == "," && toks.size() == 0) {
				// void func(int i,)
				throw NULL;
			}

			auto name = toks.back();
			auto type = toks.front();

			Declaration decl;
			decl.name = name;

			fn.decl.arguments.push_back(new FunctionArgument);
			FunctionArgument* arg = fn.decl.arguments.back();
			arg->name = name;
			arg->type = strToType(type);

			VariableDeclaration* varDecl = new VariableDeclaration{ name, arg->type, new FunctionArgumentInitializer(arg) };
			decl.data = varDecl;

			VariableDeclExp* exp = new VariableDeclExp(varDecl);

			fn.body.addDeclaration(decl);
			fn.body.addExpression(exp);

			scanner.readCursor = next.second;

			if (next.first.str == ")") {
				break;
			}

			next = scanner.peek();
		}
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
		case 0: type._isConst = false; break;
		case 1: type._isConst = true; break;
		default: return false;
		}

		switch (count(specifiers, "volatile")) {
		case 0: type._isVolatile = false; break;
		case 1: type._isVolatile = true; break;
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