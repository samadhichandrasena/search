// © 2021 the Search Authors under the MIT license. See AUTHORS for the list of authors.

#include "synth_tree.hpp"
#include "../utils/utils.hpp"
#include <cstdio>
#include <cerrno>

SynthTree::SynthTree(FILE *in, float max_err) : max_err(max_err) {
	if (!fscanf(in, "%ld", &seed)) {
	    fatal("Failed to read seed.\n");
	}
    dfpair(stdout, "random seed", "%ld", this->seed);
    dfpair(stdout, "starting AGD", "%d", AGD);
}

SynthTree::State SynthTree::initialstate() {
	State s;
	s.seed = seed;
	s.agd = AGD;
	Rand r(seed);
	double err = r.real() * max_err;
	s.h = s.agd - err * s.agd;
	s.d = s.h / MAX_COST + (s.h % MAX_COST != 0);

	return s;
}

SynthTree::Cost SynthTree::pathcost(const std::vector<State> &path, const std::vector<Oper> &ops) {
	State state = initialstate();
	Cost cost(0);
	for (int i = ops.size() - 1; i >= 0; i--) {
		State copy(state);
		//Cost c = state.agd;
		Edge e(*this, copy, ops[i]);
		assert (e.state.eq(this, path[i]));
		state = e.state;
		cost += e.cost;
	}
	assert (isgoal(state));
	return cost;
}
