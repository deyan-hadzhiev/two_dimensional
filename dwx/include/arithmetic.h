#ifndef __ARITHMETIC_H__
#define __ARITHMETIC_H__

#include "util.h"
#include <math.h>
#include <vector>
#include <algorithm>
#include <memory>

#pragma pack(push, 4)

struct BinaryEvaluationChunk {
	enum BinaryEvaluationType : uint8 {
		BET_ERROR = 0,
		BET_CONSTANT,
		BET_IDENTIFIER_X,
		BET_IDENTIFIER_Y,
		BET_IDENTIFIER_Z,
		BET_OPERAND_ADD,
		BET_OPERAND_SUBTRACT,
		BET_OPERAND_MULTIPLY,
		BET_OPERAND_DIVIDE,
		BET_OPERAND_POWER,
		BET_FUNCTION_SIN,
		BET_FUNCTION_COS,
		BET_FUNCTION_TAN,
		BET_FUNCTION_ABS,
		BET_FUNCTION_SQRT,
		BET_FUNCTION_LOG,
		BET_FUNCTION_MIN,
		BET_FUNCTION_MAX,
	} type;
	double data;
	BinaryEvaluationChunk(BinaryEvaluationType _type = BET_ERROR, double _data = 0.0)
		: type(_type)
		, data(_data)
	{}
};

#pragma pack(pop)

struct ExpressionParseError {
	int position;
	int length; //!< the length of the error part
	enum ErrorType {
		EPE_OK = 0,
		EPE_INCORRECT_PARANTHESIS,
		EPE_UNMATCHED_PARANTHESIS,
		EPE_SYNTAX,
		EPE_UNKNOWN_SYMBOL,
		EPE_UNKNOWN_VARIABLE,
		EPE_UNKNOWN_FUNCTION,
	} type;
	ExpressionParseError(ErrorType _type = EPE_OK, int _position = -1, int _length = 1)
		: type(_type)
		, position(_position)
		, length(_length)
	{}
	operator bool() const {
		return EPE_OK != type;
	}
	std::string getErrorString() const {
		std::string errorStr;
		switch (type)
		{
		case ExpressionParseError::EPE_OK:
			errorStr = "OK";
			break;
		case ExpressionParseError::EPE_INCORRECT_PARANTHESIS:
			errorStr = "Incorrect paranthesis - found ')' where it was not expected";
			break;
		case ExpressionParseError::EPE_UNMATCHED_PARANTHESIS:
			errorStr = "Unmatched paranthesis - did not find matching ')'";
			break;
		case ExpressionParseError::EPE_SYNTAX:
			errorStr = "Incorrect syntax";
			break;
		case ExpressionParseError::EPE_UNKNOWN_SYMBOL:
			errorStr = "Encountered an unknown symbol";
			break;
		case ExpressionParseError::EPE_UNKNOWN_VARIABLE:
			errorStr = "Encountered an unknown variable";
			break;
		case ExpressionParseError::EPE_UNKNOWN_FUNCTION:
			errorStr = "Encountered an unknown function";
			break;
		default:
			errorStr = "Unknown error";
			break;
		}
		return errorStr;
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
	virtual std::vector<BinaryEvaluationChunk> buildBinary() const = 0;
};

class ConstantNode : public ExpressionNode {
	double value;
public:
	ConstantNode(double val)
		: value(val)
	{}

	NodeType getType() const override { return EXP_CONSTANT; }

	double eval(const EvaluationContext& values) const override { return value; }

	std::vector<BinaryEvaluationChunk> buildBinary() const override;
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

	std::vector<BinaryEvaluationChunk> buildBinary() const override;

private:
	Variable var;
};

enum FunctionType {
	F_ERR = 0,
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

	std::vector<BinaryEvaluationChunk> buildBinary() const override;

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

	std::vector<BinaryEvaluationChunk> buildBinary() const override;

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

	std::vector<BinaryEvaluationChunk> buildBinary() const override;

private:
	std::shared_ptr<ExpressionNode> left;
	std::shared_ptr<ExpressionNode> right;
	char operand;
};

class BinaryExpressionEvaluator {
public:
	BinaryExpressionEvaluator();
	~BinaryExpressionEvaluator();

	BinaryExpressionEvaluator(const BinaryExpressionEvaluator& copy);
	BinaryExpressionEvaluator& operator=(const BinaryExpressionEvaluator& assign);

	void buildFromTree(const ExpressionNode * root);

	double eval(const EvaluationContext& context) const noexcept;
private:
	int expressionSize;
	BinaryEvaluationChunk * binaryExpression;
	double * valueStack;
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

	/** parses an expression and builds an expression tree that can be directly evaluated or can be used to build binary evaluator
	* @returns parse error if any was encountered
	*/
	ExpressionParseError buildTree(const std::string& expression);

	inline double eval(const EvaluationContext& values) const {
		return root->eval(values);
	}

	BinaryExpressionEvaluator getBinaryEvaluator() const {
		BinaryExpressionEvaluator bee;
		bee.buildFromTree(root.get());
		return bee;
	}
};

#endif // __ARITHMETIC_H__