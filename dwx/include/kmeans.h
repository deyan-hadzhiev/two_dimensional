#ifndef __KMEANS_H__
#define __KMEANS_H__

#include <vector>
#include <memory>
#include <random>

#include "vectorn.h"
#include "progress.h"

template<int d, class T, class Sum = Vector<d, T> >
std::vector<Vector<d, T> > kMeansSingle(const Vector<d, T> * valArray, const int n, const int k, const std::vector<Vector<d, T> >& means) {
	std::vector<Vector<d, T> > res;
	if (!valArray || n <= 0 || k <= 1 || n <= k || k != means.size()) {
		return res;
	}
	std::unique_ptr<Sum[]> sums(new Sum[k]);
	std::unique_ptr<int[]> sumCounts(new int[k]);
	for (int j = 0; j < k; ++j) {
		sums[j].makeZero();
		sumCounts[j] = 0;
	}
	// iterate over all the values
	for (int i = 0; i < n; ++i) {
		const Vector<d, T>& vi = valArray[i];
		// now for each value check which mean is the closest
		double minDist = (vi - means[0]).lengthSqr();
		int minIndex = 0;
		for (int j = 1; j < k; ++j) {
			const double dist = (vi - means[j]).lengthSqr();
			if (dist < minDist) {
				minDist = dist;
				minIndex = j;
			}
		}
		// now add the value to the sums of the respective index
		sums[minIndex] += vi;
		sumCounts[minIndex]++;
	}
	// now calculate all new means and add them to the resulting array
	res.resize(k);
	for (int j = 0; j < k; ++j) {
		// if there were no elements closest to the provided mean - just assign it itself
		res[j] = (sumCounts[j] > 0 ? sums[j] / static_cast<double>(sumCounts[j]) : means[j]);
	}
	return res;
}

template<int d, class T, class Sum = Vector<d, T> >
std::vector<Vector<d, T> > kMeansValues(const Vector<d, T> * valArray, const int n, const int k, const int iter, const std::vector<Vector<d, T> >& initialMeans, ProgressCallback * cb = nullptr) {
	std::vector<Vector<d, T> > res = initialMeans;
	if (!valArray || n <= 0 || k <= 1 || n <= k || iter < 1 || k != initialMeans.size()) {
		return res;
	}
	std::vector<Vector<d, T> > prevMeans;
	for (int i = 0; i < iter && (!cb || !cb->getAbortFlag()); ++i) {
		prevMeans = res;
		res = kMeansSingle<d, T, Sum>(valArray, n, k, prevMeans);
		if (cb) {
			cb->setPercentDone(i + 1, iter);
		}
	}
	return res;
}

template<int d, class T, class Sum = Vector<d, T> >
std::vector<Vector<d, T> > kMeansValues(const Vector<d, T> * valArray, const int n, const int k, const int iter, ProgressCallback * cb) {
	std::vector<Vector<d, T> > res;
	if (!valArray || n <= 0 || k <= 1 || n <= k || iter < 1) {
		return res;
	}
	std::vector<int> initialMeansIndices;
	// TODO - optimize selection of initial means
	std::random_device randDev;
	std::default_random_engine randEngine(randDev());
	std::uniform_int_distribution<int> uniformDist(0, n);
	std::srand(n * (k + 17) * (iter + 53));
	while (initialMeansIndices.size() != k) {
		bool valid = true;
		const int index = uniformDist(randEngine);
		// just check if it already is present in the initialMeans
		for (int i = 0; i < initialMeansIndices.size() && valid; ++i) {
			if (index == initialMeansIndices[i]) {
				valid = false;
			}
		}
		if (valid) {
			initialMeansIndices.push_back(index);
		}
	}
	// now add all the initial means to an array
	std::vector<Vector<d, T> > initialMeans;
	for (int i = 0; i < k; ++i) {
		initialMeans.push_back(valArray[initialMeansIndices[i]]);
	}
	// now just call the iterating function with the initial means
	res = kMeansValues<d, T, Sum>(valArray, n, k, iter, initialMeans, cb);
	return res;
}

template<int d, class T, class Sum = Vector<d, T> >
std::pair<std::vector<int>, std::vector<Vector<d, T> > > kMeans(const Vector<d, T> * valArray, const int n, const int k, const int iter, ProgressCallback * cb = nullptr) {
	std::pair<std::vector<int>, std::vector<Vector<d, T> > > res;
	if (!valArray || n <= 0 || k <= 1 || n <= k || iter < 1) {
		return res;
	}
	// first get the mean values
	res.second = kMeansValues<d, T, Sum>(valArray, n, k, iter, cb);
	// then distribute each value in the array to its appropriate mean class
	const std::vector<Vector<d, T> >& m = res.second;
	res.first.resize(n);
	for (int i = 0; i < n; ++i) {
		const Vector<d, T>& vi = valArray[i];
		// now for each value check which mean is the closest
		double minDist = Vector<d, T>(vi - m[0]).lengthSqr();
		int minIndex = 0;
		for (int j = 1; j < k; ++j) {
			const double dist = Vector<d, T>(vi - m[j]).lengthSqr();
			if (dist < minDist) {
				minDist = dist;
				minIndex = j;
			}
		}
		// now assign the appropriate class value to the resulting array
		res.first[i] = minIndex;
	}
	return res;
}

#endif // __KMEANS_H__
