#include "arithmetic.h"
#include <stack>
#include <algorithm>


std::vector<BinaryEvaluationChunk> ConstantNode::buildBinary() const {
	std::vector<BinaryEvaluationChunk> retval;
	retval.push_back(BinaryEvaluationChunk(BinaryEvaluationChunk::BET_CONSTANT, value));
	return retval;
}

std::vector<BinaryEvaluationChunk> IdentifierNode::buildBinary() const {
	std::vector<BinaryEvaluationChunk> retval;
	const BinaryEvaluationChunk::BinaryEvaluationType type = static_cast<BinaryEvaluationChunk::BinaryEvaluationType>(BinaryEvaluationChunk::BET_IDENTIFIER_X + var);
	retval.push_back(BinaryEvaluationChunk(type));
	return retval;
}

std::vector<BinaryEvaluationChunk> OneFunctionNode::buildBinary() const {
	std::vector<BinaryEvaluationChunk> retval;
	const BinaryEvaluationChunk::BinaryEvaluationType binaryType = static_cast<BinaryEvaluationChunk::BinaryEvaluationType>(BinaryEvaluationChunk::BET_FUNCTION_SIN + type - 1);
	retval.push_back(BinaryEvaluationChunk(binaryType));
	std::vector<BinaryEvaluationChunk> expressionBinary = expression->buildBinary();
	retval.insert(
		retval.end(),
		std::make_move_iterator(expressionBinary.begin()),
		std::make_move_iterator(expressionBinary.end())
	);
	return retval;
}

std::vector<BinaryEvaluationChunk> TwoFunctionNode::buildBinary() const {
	std::vector<BinaryEvaluationChunk> retval;
	const BinaryEvaluationChunk::BinaryEvaluationType binaryType = static_cast<BinaryEvaluationChunk::BinaryEvaluationType>(BinaryEvaluationChunk::BET_FUNCTION_MIN + type);
	retval.push_back(BinaryEvaluationChunk(binaryType));
	std::vector<BinaryEvaluationChunk> leftExpressionBinary = left->buildBinary();
	retval.insert(
		retval.end(),
		std::make_move_iterator(leftExpressionBinary.begin()),
		std::make_move_iterator(leftExpressionBinary.end())
	);
	std::vector<BinaryEvaluationChunk> rightExpressionBinary = right->buildBinary();
	retval.insert(
		retval.end(),
		std::make_move_iterator(rightExpressionBinary.begin()),
		std::make_move_iterator(rightExpressionBinary.end())
	);
	return retval;
}

std::vector<BinaryEvaluationChunk> CompoundNode::buildBinary() const {
	std::vector<BinaryEvaluationChunk> retval;
	BinaryEvaluationChunk::BinaryEvaluationType binaryType;
	switch(operand) {
	case '+':
		binaryType = BinaryEvaluationChunk::BET_OPERAND_ADD;
		break;
	case '-':
		binaryType = BinaryEvaluationChunk::BET_OPERAND_SUBTRACT;
		break;
	case '*':
		binaryType = BinaryEvaluationChunk::BET_OPERAND_MULTIPLY;
		break;
	case '/':
		binaryType = BinaryEvaluationChunk::BET_OPERAND_DIVIDE;
		break;
	case '^':
		binaryType = BinaryEvaluationChunk::BET_OPERAND_POWER;
		break;
	default:
		binaryType = BinaryEvaluationChunk::BET_ERROR;
		break;
	}
	retval.push_back(BinaryEvaluationChunk(binaryType));
	std::vector<BinaryEvaluationChunk> leftExpressionBinary = left->buildBinary();
	retval.insert(
		retval.end(),
		std::make_move_iterator(leftExpressionBinary.begin()),
		std::make_move_iterator(leftExpressionBinary.end())
	);
	std::vector<BinaryEvaluationChunk> rightExpressionBinary = right->buildBinary();
	retval.insert(
		retval.end(),
		std::make_move_iterator(rightExpressionBinary.begin()),
		std::make_move_iterator(rightExpressionBinary.end())
	);
	return retval;
}

static FunctionType getFunctionType(const std::string& func) {
	if (func.compare(0, 3, "sin") == 0) {
		return F_SIN;
	} else if (func.compare(0, 3, "cos") == 0) {
		return F_COS;
	} else if (func.compare(0, 3, "tan") == 0) {
		return F_TAN;
	} else if (func.compare(0, 3, "abs") == 0) {
		return F_ABS;
	} else if (func.compare(0, 4, "sqrt") == 0) {
		return F_SQRT;
	} else if (func.compare(0, 3, "log") == 0) {
		return F_LOG;
	} else if (func.compare(0, 3, "min") == 0) {
		return F_MIN;
	} else if (func.compare(0, 3, "max") == 0) {
		return F_MAX;
	} else {
		return F_ERR;
	}
	return F_ERR;
}

int ExpressionTree::getPrecedence(char op) {
	switch (op)
	{
	case '^':
		return 0;
	case '*':
	case '/':
		return 1;
	case '+':
	case '-':
		return 2;
	default:
		return 3;
	}
}

ExpressionParseError ExpressionTree::checkParanthesis() const {
	std::stack<char> st;
	std::stack<int> forwardPositions;
	bool valid = true;
	int errorPosition = -1;
	const int expressionSize = static_cast<int>(expression.size());
	for (int i = 0; i < expressionSize && valid; ++i) {
		const char c = expression[i];
		if ('(' == c) {
			st.push(c);
			forwardPositions.push(i);
		} else if (')' == c) {
			if (st.size() > 0 && st.top() == '(') {
				st.pop();
				forwardPositions.pop();
			} else {
				valid = false;
				errorPosition = i;
			}
		}
	}
	if (!valid) {
		return ExpressionParseError(ExpressionParseError::EPE_INCORRECT_PARANTHESIS, errorPosition);
	} else if (0 != st.size()) {
		return ExpressionParseError(ExpressionParseError::EPE_UNMATCHED_PARANTHESIS, forwardPositions.top());
	}
	return ExpressionParseError();
}

bool ExpressionTree::tokenize(const std::string& expr, std::vector<Token>& tokens, int offset) {
	bool valid = true;
	TokenType lastToken = TT_OPERAND;

	for (int i = 0; i < expr.size() && valid; ++i) {
		// validity check: if we have even numbers of tokens, the last token must be OPERAND.
		// if we have odd numbers of tokens the last token must be different than OPERAND
		if (tokens.size() & 1) {
			valid &= (lastToken != TT_OPERAND);
		} else {
			valid &= (lastToken == TT_OPERAND);
		}

		if (!valid) {
			throw ExpressionParseError(ExpressionParseError::EPE_SYNTAX, tokens.back().position, static_cast<int>(tokens.back().contents.size()));
		}

		const char ch = expr[i];

		// check if this is a leading expression sign
		if (i == 0 && (ch == '+' || ch == '-')) {
			// add a zero token to evaluate arithmetics correctly later
			tokens.push_back(Token(TT_CONSTANT, "0.0", -1));
			tokens.push_back(Token(TT_OPERAND, std::string(1, ch), i + offset));
			lastToken = TT_OPERAND;
			continue;
		}

		if (ch == 'x' || ch == 'y' || ch == 'z') {
			tokens.push_back(Token(TT_VARIABLE, std::string(1, ch), i + offset));
			lastToken = TT_VARIABLE;
		} else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
			tokens.push_back(Token(TT_OPERAND, std::string(1, ch), i + offset));
			lastToken = TT_OPERAND;
		} else if (ch == ')') {
			throw ExpressionParseError(ExpressionParseError::EPE_INCORRECT_PARANTHESIS, i + offset);
		} else if (ch == '(') {
			// this is a whole expression, so we must find the matching paranthesis
			int j = i;
			int left = 1;
			while (j < expr.size() - 1 && left > 0) {
				j++;
				if (expr[j] == '(')
					left++;
				else if (expr[j] == ')')
					left--;
			}
			if (left > 0 && j >= expr.size() - 1) {
				throw ExpressionParseError(ExpressionParseError::EPE_UNMATCHED_PARANTHESIS, offset + i);
			}
			
			// copy the whole string in the paranthesis
			tokens.push_back(Token(TT_EXPRESSION, expr.substr(i + 1, j - i - 1), i + offset + 1));
			lastToken = TT_EXPRESSION;
			i = j;
		} else if (isdigit(ch) || '.' == ch) {
			// a number, we must copy it
			int j = i;
			bool dot = false;
			while (isdigit(expr[j]) || (expr[j] == '.' && !dot)) {
				if (expr[j] == '.')
					dot = true;
				j++;
			}
			
			tokens.push_back(Token(TT_CONSTANT, expr.substr(i, j - i), i + offset));
			lastToken = TT_CONSTANT;
			i = j - 1;
		} else if (
			strncmp(&expr[i], "sin", 3) == 0 ||
			strncmp(&expr[i], "cos", 3) == 0 ||
			strncmp(&expr[i], "tan", 3) == 0 ||
			strncmp(&expr[i], "abs", 3) == 0 ||
			strncmp(&expr[i], "sqrt", 4) == 0 ||
			strncmp(&expr[i], "log", 3) == 0 ||
			strncmp(&expr[i], "min", 3) == 0 ||
			strncmp(&expr[i], "max", 3) == 0
			) {
			// the function must have paranthesis, so we must find the last.
			int j = i + 3;
			if (strncmp(&expr[i], "sqrt", 4) == 0)
				j = i + 4;
			int left = 1;
			while (j < expr.size() - 1 && left > 0) {
				j++;
				if (expr[j] == '(')
					left++;
				else if (expr[j] == ')')
					left--;
			}
			if (left > 0 && j >= expr.size() - 1) {
				throw ExpressionParseError(ExpressionParseError::EPE_UNMATCHED_PARANTHESIS, offset + i + 3 + (strncmp(&expr[i], "sqrt", 4) == 0 ? 1 : 0));
			}

			// copy the whole string in the paranthesis + last paranthesis
			tokens.push_back(Token(TT_FUNCTION, expr.substr(i, j - i + 1), offset + i));
			lastToken = TT_FUNCTION;
			i = j;
		} else if (
			strncmp(&expr[i], "pi", 2) == 0 ||
			strncmp(&expr[i], "e", 1) == 0
		) {
			const int size = (expr.compare(i, 2, "pi") == 0 ? 2 : 1);
			tokens.push_back(Token(TT_CONSTANT, expr.substr(i, size), offset + i));
			lastToken = TT_CONSTANT;
			i += size - 1;
		} else {
			// throw an error based on the symbol and length
			if (ch >= 'a' && ch <= 'z') {
				int size = 1;
				while (i + size < expr.size() && expr[i + size] >= 'a' && expr[i + size] <= 'z') {
					size++;
				}
				throw ExpressionParseError((size > 1 ? ExpressionParseError::EPE_UNKNOWN_FUNCTION : ExpressionParseError::EPE_UNKNOWN_VARIABLE), i + offset, size);
			} else {
				throw ExpressionParseError(ExpressionParseError::EPE_UNKNOWN_SYMBOL, offset + i);
			}
		}
	}
	// the last token must not be operand
	if (lastToken == TT_OPERAND) {
		throw ExpressionParseError(ExpressionParseError::EPE_SYNTAX, tokens.back().position, static_cast<int>(tokens.back().contents.size()));
	}
	return valid;
}

std::shared_ptr<ExpressionNode> ExpressionTree::buildToken(const Token& token) {
	std::shared_ptr<ExpressionNode> res;
	switch (token.type) {
	case ExpressionTree::TT_CONSTANT:
	{
		double contents = 0.0;
		if (token.contents == "pi") {
			contents = PI;
		} else if (token.contents == "e") {
			contents = E;
		} else {
			contents = std::stod(token.contents);
		}
		res.reset(new ConstantNode(contents));
		break;
	}
	case ExpressionTree::TT_EXPRESSION:
	{
		std::vector<Token> exprTokens;
		if (tokenize(token.contents, exprTokens, token.position)) {
			res = buildExpression(exprTokens);
		}
		break;
	}
	case ExpressionTree::TT_VARIABLE:
	{
		switch (token.contents[0]) {
		case 'x':
			res.reset(new IdentifierNode(IdentifierNode::X));
			break;
		case 'y':
			res.reset(new IdentifierNode(IdentifierNode::Y));
			break;
		case 'z':
			res.reset(new IdentifierNode(IdentifierNode::Z));
			break;
		default:
			throw ExpressionParseError(ExpressionParseError::EPE_UNKNOWN_VARIABLE, token.position);
			break;
		}
		break;
	}
	case ExpressionTree::TT_FUNCTION:
	{
		res = buildFunction(token.contents, token.position);
		break;
	}
	default:
		throw ExpressionParseError(ExpressionParseError::EPE_SYNTAX, token.position, static_cast<int>(token.contents.size()));
		break;
	}
	return res;
}

std::shared_ptr<ExpressionNode> ExpressionTree::buildExpression(const std::vector<Token>& tokens) {
	std::shared_ptr<ExpressionNode> res;
	if (tokens.size() == 1) {
		res = buildToken(tokens[0]);
	} else if (tokens.size() > 1) {
		bool error = false;
		std::stack<std::shared_ptr<ExpressionNode> > nodeStack;
		std::stack<char> opStack;
		const int tokenSize = static_cast<int>(tokens.size());
		for (int i = 0; i < tokenSize && !error; ++i) {
			if ( (i & 1) == 0 && tokens[i].type != TT_OPERAND) {
				std::shared_ptr<ExpressionNode> tokenTree = buildToken(tokens[i]);
				if (!tokenTree) {
					throw ExpressionParseError(ExpressionParseError::EPE_INTERNAL, tokens[i].position, static_cast<int>(tokens[i].contents.size()));
				}
				nodeStack.push(tokenTree);
			} else if ( (i & 1) == 1 && tokens[i].type == TT_OPERAND) {
				const char op = tokens[i].contents[0];
				const int precedence = getPrecedence(op);
				if (precedence == 3) {
					throw ExpressionParseError(ExpressionParseError::EPE_UNKNOWN_SYMBOL, tokens[i].position);
				}

				int lastPrecedence = (opStack.empty() ? 3 : getPrecedence(opStack.top()));
				if (precedence < lastPrecedence) {
					opStack.push(tokens[i].contents[0]);
				} else if (precedence == lastPrecedence) {
					const char constructOp = opStack.top();
					if (lastPrecedence > 0) {
						if (nodeStack.size() < 2 || opStack.size() < 1) {
							throw ExpressionParseError(ExpressionParseError::EPE_INTERNAL, tokens[i].position, static_cast<int>(tokens[i].contents.size()));
						}
						std::shared_ptr<ExpressionNode> right = nodeStack.top();
						nodeStack.pop();
						std::shared_ptr<ExpressionNode> left = nodeStack.top();
						nodeStack.pop();
						opStack.pop();
						std::shared_ptr<ExpressionNode> newNode(new CompoundNode(constructOp, left, right));
						nodeStack.push(newNode);
					}
					// always push the new op
					opStack.push(op);
				} else {
					while (precedence > lastPrecedence || (precedence == lastPrecedence && precedence > 0)) {
						const char constructOp = opStack.top();
						if (nodeStack.size() < 2 || opStack.size() < 1) {
							throw ExpressionParseError(ExpressionParseError::EPE_INTERNAL, tokens[i].position, static_cast<int>(tokens[i].contents.size()));
						}
						std::shared_ptr<ExpressionNode> right = nodeStack.top();
						nodeStack.pop();
						std::shared_ptr<ExpressionNode> left = nodeStack.top();
						nodeStack.pop();
						opStack.pop();
						std::shared_ptr<ExpressionNode> newNode(new CompoundNode(constructOp, left, right));
						nodeStack.push(newNode);
						lastPrecedence = (opStack.empty() ? 3 : getPrecedence(opStack.top()));
					}
					opStack.push(op);
				}
			} else {
				throw ExpressionParseError(ExpressionParseError::EPE_SYNTAX, tokens[i].position, static_cast<int>(tokens[i].contents.size()));
			}
		}

		if (!error) {
			while (!opStack.empty()) {
				const char constructOp = opStack.top();
				if (nodeStack.size() < 2 || opStack.size() < 1) {
					throw ExpressionParseError(ExpressionParseError::EPE_INTERNAL, tokens[0].position);
				}
				std::shared_ptr<ExpressionNode> right = nodeStack.top();
				nodeStack.pop();
				std::shared_ptr<ExpressionNode> left = nodeStack.top();
				nodeStack.pop();
				opStack.pop();
				std::shared_ptr<ExpressionNode> newNode(new CompoundNode(constructOp, left, right));
				nodeStack.push(newNode);
			}
			res = nodeStack.top();
		}
	}
	return res;
}

std::shared_ptr<ExpressionNode> ExpressionTree::buildFunction(const std::string& function, int offset) {
	std::shared_ptr<ExpressionNode> res;
	size_t left = function.find_first_of('(');
	size_t right = function.find_last_of(')');

	if (left != std::string::npos && right != std::string::npos) {
		FunctionType ftype = getFunctionType(function);
		if (ftype == F_ERR) {
			throw ExpressionParseError(ExpressionParseError::EPE_UNKNOWN_FUNCTION, offset, static_cast<int>(function.size()));
		} else if (ftype != F_MIN && ftype != F_MAX) {
			std::string insideExpression = function.substr(left + 1, right - left - 1);
			std::shared_ptr<ExpressionNode> insideTree;
			std::vector<Token> insideTokens;
			if (tokenize(insideExpression, insideTokens, static_cast<int>(offset + left + 1))) {
				insideTree = buildExpression(insideTokens);
			}

			if (insideTree) {
				res.reset(new OneFunctionNode(ftype, insideTree));
			}
		} else {
			size_t comaPos = left + 1;
			int bracketLayer = 0;
			bool found = false;
			while (comaPos < right) {
				if (function[comaPos] == ',' && bracketLayer == 0) {
					found = true;
					break;
				} else if (function[comaPos] == '(') {
					bracketLayer++;
				} else if (function[comaPos] == ')') {
					bracketLayer--;
				}
				comaPos++;
			}

			if (found) {
				std::string leftExpr = function.substr(left + 1, comaPos - left - 1);
				std::string rightExpr = function.substr(comaPos + 1, right - comaPos - 1);

				std::vector<Token> leftTokens;
				std::vector<Token> rightTokens;

				if (tokenize(leftExpr, leftTokens, static_cast<int>(offset + left + 1)) && tokenize(rightExpr, rightTokens, static_cast<int>(offset + comaPos + 1))) {
					std::shared_ptr<ExpressionNode> leftSubTree = buildExpression(leftTokens);
					std::shared_ptr<ExpressionNode> rightSubTree = buildExpression(rightTokens);

					if (leftSubTree && rightSubTree) {
						res.reset(new TwoFunctionNode(ftype, leftSubTree, rightSubTree));
					}
				}
			} else {
				throw ExpressionParseError(ExpressionParseError::EPE_MISSING_COMMA, static_cast<int>(offset + left + 1), static_cast<int>(function.size() - left));
			}
		}
	} else {
		throw ExpressionParseError(ExpressionParseError::EPE_INTERNAL, offset, static_cast<int>(function.size()));
	}
	return res;
}

ExpressionParseError ExpressionTree::buildTree(const std::string& expr) {
	expression = expr;
	// remove whitespace
	expression.erase(std::remove_if(expression.begin(), expression.end(), ::isspace), expression.end());
	// transform to lower
	std::transform(expression.begin(), expression.end(), expression.begin(), ::tolower);

	ExpressionParseError parseError;
	parseError = checkParanthesis();
	if (!parseError) {
		std::vector<Token> firstLayer;
		try {
			// tokenize the first layer of the epxression
			tokenize(expression, firstLayer, 0);
			// now build an expression from the first layer
			root = buildExpression(firstLayer);
		} catch (const ExpressionParseError& epr) {
			parseError = epr;
			root.reset();
		}
	}
	return parseError;
}

BinaryExpressionEvaluator::BinaryExpressionEvaluator()
	: expressionSize(0)
	, binaryExpression(nullptr)
	, valueStack(nullptr)
{}

BinaryExpressionEvaluator::~BinaryExpressionEvaluator() {
	delete[] valueStack;
	delete[] binaryExpression;
}

BinaryExpressionEvaluator::BinaryExpressionEvaluator(const BinaryExpressionEvaluator & copy)
	: expressionSize(copy.expressionSize)
	, binaryExpression(new BinaryEvaluationChunk[copy.expressionSize])
	, valueStack(new double[copy.expressionSize])
{
	memcpy(binaryExpression, copy.binaryExpression, copy.expressionSize * sizeof(BinaryEvaluationChunk));
}

BinaryExpressionEvaluator& BinaryExpressionEvaluator::operator=(const BinaryExpressionEvaluator & assign) {
	if (this != &assign) {
		delete[] valueStack;
		delete[] binaryExpression;
		expressionSize = assign.expressionSize;
		binaryExpression = new BinaryEvaluationChunk[expressionSize];
		memcpy(binaryExpression, assign.binaryExpression, assign.expressionSize * sizeof(BinaryEvaluationChunk));
		valueStack = new double[expressionSize];
	}
	return *this;
}

void BinaryExpressionEvaluator::buildFromTree(const ExpressionNode * root) {
	if (root) {
		std::vector<BinaryEvaluationChunk> expressionVec = root->buildBinary();
		// reverse the expression to be ready for evaluation in postfix polish notation
		std::reverse(expressionVec.begin(), expressionVec.end());
		const int newSize = static_cast<int>(expressionVec.size());
		if (expressionSize != newSize) {
			expressionSize = newSize;
			delete[] binaryExpression;
			binaryExpression = new BinaryEvaluationChunk[expressionSize];
			delete[] valueStack;
			valueStack = new double[expressionSize];
		}
		for (int i = 0; i < expressionSize; ++i) {
			binaryExpression[i] = expressionVec[i];
		}
	}
}

double BinaryExpressionEvaluator::eval(const EvaluationContext & context) const noexcept {
	int stackTop = -1;
	const int evalSize = expressionSize;
	for (int i = 0; i < evalSize; ++i) {
		const BinaryEvaluationChunk& symbol = binaryExpression[i];
		switch (symbol.type) {
		case (BinaryEvaluationChunk::BET_CONSTANT):
			valueStack[++stackTop] = symbol.data;
			break;
		case (BinaryEvaluationChunk::BET_IDENTIFIER_X):
			valueStack[++stackTop] = context.x;
			break;
		case (BinaryEvaluationChunk::BET_IDENTIFIER_Y):
			valueStack[++stackTop] = context.y;
			break;
		case (BinaryEvaluationChunk::BET_IDENTIFIER_Z):
			valueStack[++stackTop] = context.z;
			break;
		case (BinaryEvaluationChunk::BET_OPERAND_ADD):
			--stackTop;
			valueStack[stackTop] += valueStack[stackTop + 1];
			break;
		case (BinaryEvaluationChunk::BET_OPERAND_SUBTRACT):
			--stackTop;
			valueStack[stackTop] = valueStack[stackTop + 1] - valueStack[stackTop];
			break;
		case (BinaryEvaluationChunk::BET_OPERAND_MULTIPLY):
			--stackTop;
			valueStack[stackTop] *= valueStack[stackTop + 1];
			break;
		case (BinaryEvaluationChunk::BET_OPERAND_DIVIDE):
			--stackTop;
			valueStack[stackTop] = valueStack[stackTop + 1] / valueStack[stackTop];
			break;
		case (BinaryEvaluationChunk::BET_OPERAND_POWER):
			--stackTop;
			valueStack[stackTop] = pow(valueStack[stackTop + 1], valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_SIN):
			valueStack[stackTop] = sin(valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_COS):
			valueStack[stackTop] = cos(valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_TAN):
			valueStack[stackTop] = tan(valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_ABS):
			valueStack[stackTop] = abs(valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_SQRT):
			valueStack[stackTop] = sqrt(valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_LOG):
			valueStack[stackTop] = log(valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_MIN):
			--stackTop;
			valueStack[stackTop] = std::min(valueStack[stackTop + 1], valueStack[stackTop]);
			break;
		case (BinaryEvaluationChunk::BET_FUNCTION_MAX):
			--stackTop;
			valueStack[stackTop] = std::max(valueStack[stackTop + 1], valueStack[stackTop]);
			break;
		default:
			break;
		}
	}
	return valueStack[0];
}
