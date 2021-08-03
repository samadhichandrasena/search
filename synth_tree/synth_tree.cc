// © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.

#include "synth_tree.hpp"
#include "../utils/utils.hpp"
#include <cstdio>
#include <cerrno>

SynthTree::SynthTree(FILE *in, const char *cost) : cost(cost) {
    fatal("Unimplemented.");
}

SynthTree::State SynthTree::initialstate() {
	State s;

    fatal("Unimplemented.");

	return s;
}

SynthTree::Cost SynthTree::pathcost(const std::vector<State> &path, const std::vector<Oper> &ops) {
	State state = initialstate();
	Cost cost(0);
	for (int i = ops.size() - 1; i >= 0; i--) {
		State copy(state);
		Edge e(*this, copy, ops[i]);
		assert (e.state.eq(this, path[i]));
		state = e.state;
		cost += e.cost;
	}
	assert (isgoal(state));
	return cost;
}
