#include "blocksworld.hpp"
#include "../utils/utils.hpp"
#include <cstdio>
#include <cerrno>
#include <iostream>

const Blocksworld::Oper Blocksworld::Nop;

Blocksworld::Blocksworld(FILE *in) {
	unsigned int Nblocks;
	if (fscanf(in, "%u\n", &Nblocks) != 1)
		fatalx(errno, "Failed to read the number of blocks");

    if (!fscanf(in, "What each block is on:\n")) {
	  fatal("Missing block header line in input file.\n");
	}
    for (unsigned int i = 0; i < Nblocks; i++) {
        if (fscanf(in, "%hhu\n", &init[i]) != 1)
            fatalx(errno, "Failed to read basic block number %d", i);
    }

    if (!fscanf(in, "Goal:\n")) {
	  fatal("Missing goal header line in input file.\n");
	}
	for (unsigned int i = 0; i < Nblocks; i++) {
        if (fscanf(in, "%hhu\n", &goal[i]) != 1)
            fatalx(errno, "Failed to read basic block number %d", i);
	}

    for(Block from = 0; from<Nblocks; from++){
#ifndef DEEP
        for(Block to = 0; to < Nblocks; to++){
            if(from == to) movelibrary[(from)*Nblocks + to] = Move(from+1, 0);
            else movelibrary[(from)*Nblocks + to] = Move(from+1, to+1);
        }
#else
        for(Block to = 0; to <= Nblocks; to++){
            movelibrary[(from)*(Nblocks+1) + to] = Move(from+1, to);
        }
#endif
    }
}

Blocksworld::State Blocksworld::initialstate() {
	State s;
	for (Block i = 0; i < Nblocks; i++){
        s.below[i] = init[i];
        if(s.below[i]!=0) s.above[s.below[i]-1] = i+1;
    }
	s.h = noop(s.above, s.below);
#ifdef DEEP
    s.h *= 2;
#endif
    s.d = s.h;
	return s;
    }

Blocksworld::Cost Blocksworld::pathcost(const std::vector<State> &path, const std::vector<Oper> &ops) {
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
