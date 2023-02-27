// Copyright © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.
#pragma once

#include <vector>
#include <boost/optional.hpp>
#include <math.h>

void fatal(const char*, ...);

// TODO: changes needed to pushdown functions

// Implements a min-max heap, assuming that pred(a, b) -> a < b
// and !pred(a, b) -> a >= b, for some ordering system.

template <class Ops, class Elm> class MinMaxHeap {
public:
	// push pushes a new element into the heap in
	// O(lg n) time.
	void push(Elm e) {
		heap.push_back(e);
		Ops::setind(e, heap.size() - 1);
		pullup(heap.size() - 1);
	}

	// pop pops the front element from the heap and
	// returns it in O(lg n) time.
	boost::optional<Elm> pop_min() {
		if (heap.size()== 0)
			return boost::optional<Elm>();

		Elm res = heap[0];
		if (heap.size() > 1) {
			heap[0] = heap.back();
			heap.pop_back();
			Ops::setind(heap[0], 0);
			pushdown(0);
		} else {
			heap.pop_back();
		}
		Ops::setind(res, -1);

		return boost::optional<Elm>(res);
	}

	// pop pops the front element from the heap and
	// returns it in O(lg n) time.
	boost::optional<Elm> pop_max() {
		if (heap.size() == 0)
			return boost::optional<Elm>();

		Elm res = heap[0];
		if (heap.size() > 1) {
			int m = heap.size() == 2 || !Ops::pred(heap[1], heap[2]) ? 1 : 2;
			res = heap[m];
			heap[m] = heap.back();
			heap.pop_back();
			Ops::setind(heap[m], m);
			pushdown(m);
		} else {
			heap.pop_back();
		}
		Ops::setind(res, -1);

		return boost::optional<Elm>(res);
	}

	// front returns the front element of the heap if it is
	// not empty and an empty option if the heap is empty.
	// O(1) time.
	boost::optional<Elm> front() {
		if (heap.size() == 0)
			return boost::optional<Elm>();
		return boost::optional<Elm>(heap[0]);
	}

	// back returns the last-ordered element of the heap if it is
	// not empty and an empty option if the heap is empty.
	// O(1) time.
	boost::optional<Elm> back() {
		if (heap.size() == 0)
			return boost::optional<Elm>();
		return boost::optional<Elm>(heap.size() == 2 || heap.at(1) > heap.at(2) ? heap[1] : heap[2]);
	}

	// update updates the element in the heap at the given
	// index.  This should be called whenever the priority
	// of an element changes.  O(lg n) time.
	void update(long i) {
		if (i < 0 || (unsigned int) i >= heap.size())
			fatal("Updating an invalid heap index: %ld, size=%lu\n", i, heap.size());
		i = pullup(i);
		pushdown(i);
	}

	// pushupdate either pushes the element or updates it's
	// position in the queue, given the element and i, it's
	// index value.  Note that the index value must be tracked
	// properly, i.e., i < 0 means that the element is not in
	// the priority queue and i  >= 0 means that the element
	// is at the given index. O(lg n) time.
	void pushupdate(Elm e, long i) {
		if (i  < 0)
			push(e);
		else
			update(i);
	}

	// empty returns true if the heap is empty and
	// false otherwise.
	bool empty() const { return heap.empty(); }

	// clear clears all of the elements from the heap
	// leaving it empty.
	void clear() { heap.clear(); }

 	// size returns the number of entries in the heap.
	long size() const { return heap.size(); }

	// at returns the element of the heap at the given
	// index.
	Elm at(long i) {
		assert (i < size());
		return heap[i];
	}

	// data returns the raw vector used to back the
	// heap.  If you mess with this then you must
	// reinit the heap afterwards to ensure that the
	// heap property still holds.
	std::vector<Elm> &data() { return heap; }

	// reinit reinitialize the heap property in O(n) time.
	void reinit() {
		if (heap.size() <= 0)
			return;

		for (unsigned int i = 0; i < heap.size(); i++)
			Ops::setind(heap[i], i);

		for (long i = heap.size() / 2; ; i--) {
			pushdown(i);
			if (i == 0)
				break;
		}
	}

	// append appends the vector of elements to the heap
	// and ensures that the heap property holds after.
	void append(const std::vector<Elm> &elms) {
		heap.insert(heap.end(), elms.begin(), elms.end());
		reinit();
	}

	long pushdown(long i) {
	  if (minnode(i)) {
		i = pushdown_min(i);
	  } else {
		i = pushdown_max(i);
	  }

	  return i;
	}

	long pushdown_max(long i) {
	  long l = left(i), r = right(i);
	  long gc[] = {left(l), right(l), left(r), right(r)};

	  if (l < size()) { // i has children
		// get max among all children and grandchildren
		int m = max_pred(l, r);
		for (int x : gc) {
		  m = max_pred(m, x);
		}
		
		if (m != l and m != r) { // m is a grandchild of i
		  if (Ops::pred(heap[i], heap[m])) { // heap[m] > heap[i]
			swap(m, i);
			if (Ops::pred(heap[m], heap[parent(m)])) {
			  // heap[m] < heap[parent(m)]
			  swap(m, parent(m));
			}
			i = pushdown_max(m);
		  }
		} else { // max was a child of i
		  if (Ops::pred(heap[i], heap[m])) { // heap[m] > heap[i]
			swap(i, m);
			i = m;
		  }
		}
	  }

	  return i;
	}

	long pushdown_min(long i) {
	  long l = left(i), r = right(i);
	  long gc[] = {left(l), right(l), left(r), right(r)};

	  if (l < size()) { // i has children
		// get min among all children and grandchildren
		int m = min_pred(l, r);
		for (int x : gc) {
		  m = min_pred(m, x);
		}
		
		if (m != l and m != r) { // m is a grandchild of i
		  if (Ops::pred(heap[m], heap[i])) { // heap[m] < heap[i]
			swap(m, i);
			if (Ops::pred(heap[parent(m)], heap[m])) {
			  // heap[m] > heap[parent(m)]
			  swap(m, parent(m));
			}
			i = pushdown_min(m);
		  }
		} else { // max was a child of i
		  if (Ops::pred(heap[m], heap[i])) { // heap[m] < heap[i]
			swap(i, m);
			i = m;
		  }
		}
	  }

	  return i;
	}

private:
	friend bool minmaxheap_push_test();
	friend bool minmaxheap_pop_test();

	long parent(long i) { return (i - 1) / 2; }

	long left(long i) { return 2 * i + 1; }

	long right(long i) { return 2 * i + 2; }

	long level(long i) { return (long)log2(i+1); }

	long minnode(long i) { return i == 0 || level(i) % 2 == 0; }

	long maxnode(long i) { return i != 0 || level(i) % 2 != 0; }

	// Returns the max between l and r by pred ordering. Assumes l exists.
	long max_pred(long l, long r) {
	  return (r >= size() || Ops::pred(heap[r], heap[l])) ? l : r;
	}

	// Returns the min between l and r by pred ordering. Assumes l exists.
	long min_pred(long l, long r) {
	  return (r >= size() || Ops::pred(heap[l], heap[r])) ? l : r;
	}

	long pullup(long i) {
		if (i == 0)
			return i;
		long p = parent(i);
		if (minnode(i)) { // min node
		  if (Ops::pred(heap[p], heap[i])) { // heap[i] > heap[p]
			swap(i, p);
			i = pullup_max(p);
		  } else { // heap[i] <= heap[p]
			i = pullup_min(i);
		  }
		} else { // max node
		  if (Ops::pred(heap[i], heap[p])) { // heap[i] < heap[p]
			swap(i, p);
			i = pullup_min(p);
		  } else { // heap[i] >= heap[p]
			i = pullup_max(i);
		  }
		}
		return i;
	}

	long pullup_min(long i) {
	  if (parent(i) <= 0) {
		return i;
	  }
	  int gp = parent(parent(i));
	  if (Ops::pred(heap[i], heap[gp])) { // heap[i] < heap[p]
		swap(i, gp);
		i = pullup_min(gp);
	  }
	  return i;
	}

	long pullup_max(long i) {
	  if (parent(parent(i)) <= 0) {
		return i;
	  }
	  int gp = parent(parent(i));
	  if (Ops::pred(heap[gp], heap[i])) { // heap[i] > heap[p]
		swap(i, gp);
		return pullup_max(gp);
	  }
	  return i;
	}

	void swap(long i, long j) {
		Ops::setind(heap[i], j);
		Ops::setind(heap[j], i);
		Elm tmp = heap[i];
		heap[i] = heap[j];
		heap[j] = tmp;
	}

	std::vector<Elm> heap;
};
