#ifndef __ARITHMETIC_H__
#define __ARITHMETIC_H__

#include "util.h"
#include <math.h>
#include <vector>
#include <algorithm>
#include <memory>

struct ExpressionParseError {
	int position;
	enum ErrorType {
		EPE_OK = 0,
		EPE_INCORRECT_PARANTHESIS,
		EPE_UNMATCHED_PARANTHESIS,
		EPE_SYNTAX,
		EPE_UNKNOWN_VARIABLE,
		EPE_UNKNOWN_FUNCTION,
	} type;
	ExpressionParseError(ErrorType _type = EPE_OK, int _position = -1)
		: type(_type)
		, position(_position)
	{}
	operator bool() const {
		return EPE_OK != type;
	}
};

class EvaluationContext {
public:
	double x, y, z;
	EvaluationContext(double _x, double _y, double _z)
		: x(_x)
		, y(_y)
		, z(_z)
	{}
};

/** @class ExpNode
* @brief a class representing interface to a single node in our expression tree.
*/
class ExpressionNode {
public:
	virtual ~ExpressionNode() {}

	enum NodeType {
		EXP_CONSTANT, //!< a constant double number
		EXP_IDENTIFIER, //!< a variable
		EXP_FUNCTION_SINGLE, //!< a function with 1 argument
		EXP_FUNCTION_TWO, //!< a two variable function
		EXP_COMPOUND, //!< a compound node of two other nodes and operator
	};

	virtual NodeType getType() const = 0;
	virtual double eval(const EvaluationContext& values) const = 0;
};

class ConstantNode : public ExpressionNode {
	double value;
public:
	ConstantNode(double val)
		: value(val)
	{}

	NodeType getType() const override { return EXP_CONSTANT; }

	double eval(const EvaluationContext& values) const override { return value; }
};

class IdentifierNode : public ExpressionNode {
public:
	enum Variable {
		X = 0,
		Y,
		Z,
	};

	IdentifierNode(Variable id) : var(id) {}

	NodeType getType() const override { return EXP_IDENTIFIER; }

	double eval(const EvaluationContext& values) const override {
		switch (var) {
		case IdentifierNode::X:
			return values.x;
		case IdentifierNode::Y:
			return values.y;
		case IdentifierNode::Z:
			return values.z;
		default:
			return 0.0;
		}
	}

private:
	Variable var;
};

enum FunctionType {
	F_ERR,
	F_SIN,
	F_COS,
	F_TAN,
	F_ABS,
	F_SQRT,
	F_LOG, //!< natural logarithm
	F_MIN,
	F_MAX,
};

class OneFunctionNode : public ExpressionNode {
public:
	OneFunctionNode(FunctionType type, std::shared_ptr<ExpressionNode> _expression)
		: type(type)
		, expression(_expression)
	{}

	NodeType getType() const override { return EXP_FUNCTION_SINGLE; }

	double eval(const EvaluationContext& values) const override {
		switch (type) {
		case F_SIN:
			return sin(expression->eval(values));
		case F_COS:
			return cos(expression->eval(values));
		case F_TAN:
			return tan(expression->eval(values));
		case F_ABS:
			return abs(expression->eval(values));
		case F_SQRT:
			return sqrt(expression->eval(values));
		case F_LOG:
			return log(expression->eval(values));
		default:
			return 0.0;
		}
	}

private:
	std::shared_ptr<ExpressionNode> expression;
	FunctionType type;
};

class TwoFunctionNode : public ExpressionNode {
public:
	TwoFunctionNode(FunctionType t, std::shared_ptr<ExpressionNode> l, std::shared_ptr<ExpressionNode> r)
		: type(t)
		, left(l)
		, right(r)
	{}

	NodeType getType() const override { return EXP_FUNCTION_TWO; }

	double eval(const EvaluationContext& values) const override {
		switch (type) {
		case F_MIN:
			return std::min(left->eval(values), right->eval(values));
		case F_MAX:
			return std::max(left->eval(values), right->eval(values));
		default:
			return 0.0;
		}
	}

private:
	std::shared_ptr<ExpressionNode> left;
	std::shared_ptr<ExpressionNode> right;
	FunctionType type;
};

class CompoundNode : public ExpressionNode {
public:
	CompoundNode(char op, std::shared_ptr<ExpressionNode> l, std::shared_ptr<ExpressionNode> r)
		: operand(op)
		, left(l)
		, right(r)
	{}

	NodeType getType() const override { return EXP_COMPOUND; }

	double eval(const EvaluationContext& values) const override {
		switch (operand) {
		case '+':
			return left->eval(values) + right->eval(values);
		case '-':
			return left->eval(values) - right->eval(values);
		case '*':
			return left->eval(values) * right->eval(values);
		case '/':
			return left->eval(values) / right->eval(values);
		case '^':
			return pow(left->eval(values), right->eval(values));
		default:
			return 0.0;
		}
	}

private:
	std::shared_ptr<ExpressionNode> left;
	std::shared_ptr<ExpressionNode> right;
	char operand;
};

class ExpressionTree {
	enum TokenType {
		TT_CONSTANT,
		TT_EXPRESSION,
		TT_VARIABLE,
		TT_FUNCTION,
		TT_OPERAND,
	};

	struct Token {
		Token(TokenType t, const std::string& c)
			: type(t)
			, contents(c)
		{}
		TokenType type;
		std::string contents;
	};

	std::string expression;
	std::shared_ptr<ExpressionNode> root;

	// check for valid paranthesis
	ExpressionParseError checkParanthesis() const;

	// returns the precedence of the operations ^ => 0, * / => 1, + - => 2
	static int getPrecedence(char op);

	// constructs one layer of tokens and saves it in 'tokens'
	// returns false if expression has an invalid tokens
	static bool tokenize(const std::string& expr, std::vector<Token>& tokens);

	// builds a tree from a single token.
	// if the token is expression of function, it calls build expression for it.
	static std::shared_ptr<ExpressionNode> buildToken(const Token& token);

	// builds a tree from a vector of tokens.
	static std::shared_ptr<ExpressionNode> buildExpression(const std::vector<Token>& tokens);

	// builds a tree from a function expression string
	static std::shared_ptr<ExpressionNode> buildFunction(const std::string& function);
public:
	ExpressionTree()
		: root()
		, expression("")
	{}

	std::string getExpression() const { return expression; }

	bool buildTree(const std::string& expression);

	inline double eval(const EvaluationContext& values) const {
		return root->eval(values);
	}
};

#endif // __ARITHMETIC_H__