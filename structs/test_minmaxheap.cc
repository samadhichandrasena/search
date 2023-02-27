// © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.

#include "../utils/utils.hpp"
#include "minmaxheap.hpp"
#include <cstdlib>

struct UIntOps {
	static void setind(unsigned int, int) {}
	static bool pred(unsigned int a, unsigned int b) { return a < b; }
};

enum { N = 10000 };

bool minmaxheap_push_test() {
	bool res = true;
	MinMaxHeap<UIntOps, unsigned int> pq;
	unsigned int data[N];

	for (unsigned int i = 0; i < N; i++) {
		data[i] = rand() % 100;
		pq.push(data[i]);
		if (pq.heap.size() != (unsigned long) i+1) {
			testpr("Expected fill of %u got %lu\n", i+1, pq.heap.size());
			res = false;
		}
	}
 
	return res;
}

bool minmaxheap_pop_test() {
	bool res = true;
	MinMaxHeap<UIntOps, unsigned int> pq;
	unsigned int data[N];

	for (int i = 0; i < N; i++) {
		data[i] = rand() % 100;
		pq.push(data[i]);
	}

	boost::optional<unsigned int> e = pq.pop_min();
	if (!e) {
		testpr("Empty pop_min");
		res = false;
	}
	int min = *e;

	e = pq.pop_max();
	if (!e) {
		testpr("Empty pop_max");
		res = false;
	}
	int max = *e;

	for (unsigned int i = 2; i < N; i++) {
		e = pq.pop_min();
		if (!e) {
			testpr("Empty pop_min");
			res = false;
		}

		int m = *e;
		if (pq.heap.size() != (unsigned long) N - (i+1)) {
			testpr("Expected fill of %u got %lu\n", N-(i+1), pq.heap.size());
			res = false;
		}

		if (m < min) {
			testpr("%d came out as min after %d\n", m, min);
			res = false;
		}
		min = m;
		i++;

		e = pq.pop_max();
		if (!e) {
			testpr("Empty pop_max");
			res = false;
		}

		m = *e;
		if (pq.heap.size() != (unsigned long) N - (i+1)) {
			testpr("Expected fill of %u got %lu\n", N-(i+1), pq.heap.size());
			res = false;
		}

		if (m > max) {
			testpr("%d came out as max after %d\n", m, max);
			res = false;
		}
		max = m;
	}
	return res;
}

unsigned int *data;

void minmaxheap_push_bench(unsigned long n, double *strt, double *end) {
	MinMaxHeap<UIntOps, unsigned int> pq;
	data = new unsigned int[n];

	for (unsigned long i = 0; i < n; i++)
		data[i] = rand() % 1000;

	*strt = walltime();

	for (unsigned long i = 0; i < n; i++)
		pq.push(data[i]);

	*end = walltime();
 
	delete[] data;
}

void minmaxheap_pop_bench(unsigned long n, double *strt, double *end) {
	MinMaxHeap<UIntOps, unsigned int> pq;
	data = new unsigned int[n];

	for (unsigned long i = 0; i < n; i++) {
		data[i] = rand() % 1000;
		pq.push(data[i]);
	}


	*strt = walltime();
 
	for (unsigned long i = 0; i < n; i++) {
	  i%2 == 0 ? pq.pop_min() : pq.pop_max();
	}

	*end = walltime();
 
	delete[] data;
}
