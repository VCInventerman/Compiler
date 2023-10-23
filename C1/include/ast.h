#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <string_view>
#include <any>
#include <stdexcept>
#include <memory>
#include <iostream>

#include "token.h"


struct AstNode {
	std::unique_ptr<AstNode> leftChild;
	std::unique_ptr<AstNode> rightChild;

	Token tok;

private:
	AstNode() {}

public:
	static std::unique_ptr<AstNode> makeLeaf(Token tok_) {
		std::unique_ptr<AstNode> ptr(new AstNode());

		ptr->tok = tok_;

		return ptr;
	}

	static std::unique_ptr<AstNode> makeParent(Token tok_, std::unique_ptr<AstNode>&& left_, std::unique_ptr<AstNode>&& right_) {
		std::unique_ptr<AstNode> ptr(new AstNode());

		ptr->tok = tok_;
		ptr->leftChild = std::move(left_);
		ptr->rightChild = std::move(right_);

		return ptr;
	}
};

int interpretAst(AstNode* root) {
	int leftVal = 0;
	int rightVal = 0;

	if (root->leftChild) {
		leftVal = interpretAst(root->leftChild.get());
	}
	if (root->rightChild) {
		rightVal = interpretAst(root->rightChild.get());
	}

	std::cout << "RUN " << OPERATOR_TOKENS[(int)root->tok.type] << ' ' << leftVal << ' ' << rightVal << '\n';

	switch (root->tok.type) {
		case TokenType::PLUS: return leftVal + rightVal;
		case TokenType::MINUS: return leftVal - rightVal;
		case TokenType::MULT: return leftVal * rightVal;
		case TokenType::DIV: return leftVal / rightVal;
		case TokenType::MODULO: return leftVal % rightVal;
		case TokenType::PRIMITIVE: return std::any_cast<int>(root->tok.defaultValue);
		default:
			std::cout << "Unknown token type!\n";
	}

	throw 0;
}

#endif