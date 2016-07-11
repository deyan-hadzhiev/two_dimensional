#include <iostream>
#include <chrono>

#include "util.h"
#include "arithmetic.h"

int64 testDirectEvaluation(const char * expr, const int evaluations, std::unique_ptr<double[]>& output, EvaluationContext start, EvaluationContext step) {
	ExpressionTree et;
	int64 evalDuration = 0;
	if (et.buildTree(expr)) {
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

int main(int argc, char* argv[]) {
	const char expression[] = "x +  y ^ 2 - cos(z) + ((x * y)/(z ^ 2 + (x * (pi / 2)) + (0 - y * (pi / 2))) ) * pi";
	const int size = 1000000;
	std::unique_ptr<double[]> directOutput;
	const EvaluationContext start(0, 0, 1);
	const EvaluationContext step(0.001, 0.001, 0.001);
	std::cout << expression << std::endl;
	int64 oldEvaluation = testDirectEvaluation(expression, size, directOutput, start, step);
	std::cout << "Direct duration: " << oldEvaluation << std::endl;
	system("pause");
	return 0;
}
