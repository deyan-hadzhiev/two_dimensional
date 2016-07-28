#include <iostream>
#include <chrono>

#include "util.h"
#include "arithmetic.h"

int64 testDirectEvaluation(const char * expr, const int evaluations, std::unique_ptr<double[]>& output, EvaluationContext start, EvaluationContext step) {
	ExpressionTree et;
	int64 evalDuration = 0;
	if (!et.buildTree(expr)) {
		output.reset(new double[evaluations]);
		EvaluationContext currContext = start;
		const auto start = std::chrono::steady_clock::now();
		for (int i = 0; i < evaluations; ++i) {
			output[i] = et.eval(currContext);
			currContext.x += step.x;
			currContext.y += step.y;
			currContext.z += step.z;
		}
		const auto end = std::chrono::steady_clock::now();
		evalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}
	return evalDuration;
}

int64 testBinaryEvaluation(const char * expr, const int evaluations, std::unique_ptr<double[]>& output, EvaluationContext start, EvaluationContext step) {
	ExpressionTree et;
	int64 evalDuration = 0;
	if (!et.buildTree(expr)) {
		output.reset(new double[evaluations]);
		EvaluationContext currContext = start;
		BinaryExpressionEvaluator bee = et.getBinaryEvaluator();
		const auto start = std::chrono::steady_clock::now();
		for (int i = 0; i < evaluations; ++i) {
			output[i] = bee.eval(currContext);
			currContext.x += step.x;
			currContext.y += step.y;
			currContext.z += step.z;
		}
		const auto end = std::chrono::steady_clock::now();
		evalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}
	return evalDuration;
}

int main(int argc, char* argv[]) {
	const char expression[] = "(((x - 20) * cos (sqrt(2) / 2) + (y - 10) * sin(sqrt(2) / 2)) / 15 ) ^ 2 + (((x - 20) * sin (sqrt(2) / 2) - (y - 10) * cos(sqrt(2) / 2)) / 35 ) ^ 2 - 1 + (((x - 20) * cos (sqrt(2) / 2) + (y - 10) * sin(sqrt(2) / 2)) / 15 ) ^ 2 + (((x - 20) * sin (sqrt(2) / 2) - (y - 10) * cos(sqrt(2) / 2)) / 35 ) ^ 2 - 1 + (((x - 20) * cos (sqrt(2) / 2) + (y - 10) * sin(sqrt(2) / 2)) / 15 ) ^ 2 + (((x - 20) * sin (sqrt(2) / 2) - (y - 10) * cos(sqrt(2) / 2)) / 35 ) ^ 2 - 1 + (((x - 20) * cos (sqrt(2) / 2) + (y - 10) * sin(sqrt(2) / 2)) / 15 ) ^ 2 + (((x - 20) * sin (sqrt(2) / 2) - (y - 10) * cos(sqrt(2) / 2)) / 35 ) ^ 2 - 1";
	const int size = 10000000;
	std::unique_ptr<double[]> directOutput;
	const EvaluationContext start(0, 0, 1);
	const EvaluationContext step(0.001, 0.001, 0.001);
	std::cout << expression << std::endl;
	int64 oldEvaluation = testDirectEvaluation(expression, size, directOutput, start, step);
	std::cout << "Direct duration: " << oldEvaluation << std::endl;
	std::unique_ptr<double[]> binaryOutput;
	int64 binaryEvaluation = testBinaryEvaluation(expression, size, binaryOutput, start, step);
	std::cout << "Binary duration: " << binaryEvaluation << std::endl;
	int errors = 0;
	for (int i = 0; i < size; ++i) {
		if (abs(directOutput[i] - binaryOutput[i]) > Eps) {
			printf("%20.12lf == %20.12lf\n", directOutput[i], binaryOutput[i]);
			errors++;
		}
	}
	std::cout << "Errors: " << errors << std::endl;
	system("pause");
	return 0;
}
