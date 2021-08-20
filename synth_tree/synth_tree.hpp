#include "../utils/utils.hpp"
#include <cstdio>
#include <limits.h>

#define BF 25
#define AGD 500
#define MAX_COST 10

class SynthTree {
public:
	typedef int Cost;

	typedef long Oper;

	static const Oper Nop = -1;

    SynthTree(FILE*, float);

	struct State {
	public:
		bool eq(const SynthTree*, const State &o) const {
			return seed == o.seed && agd == o.agd && h == o.h;
		}

		unsigned long hash(const SynthTree*) const {
		    return seed;
		}

		double err() const {
		    if(agd > 0)
			  return (agd - h) / (double)agd;
			else
			  return 0;
		}
        

	private:
        friend class SynthTree;

		Cost h;
		Cost d;
	    int seed;
	    Cost agd;
	};

    typedef State PackedState;

	unsigned long hash(const PackedState &s) const {
		return s.seed;
	}

	// Get the initial state.
	State initialstate(void);

	// Get the heuristic.
	Cost h(const State &s) const {
		return s.h;
	}

	// Get a distance estimate.
	Cost d(const State &s) const {
		return s.d;
	}

	// Is the given state a goal state?
	bool isgoal(const State &s) const {
		return s.agd == 0;
	}

	// Operators implements an vector of the applicable
	// operators for a given state.
	struct Operators {
	    
	    Oper ops[BF];
	    
	    Operators(const SynthTree &d, const State &s) {
		    Rand r(s.seed);
			
		    for(int i = 0; i < BF; i++) {
			  ops[i] = r.integer(0, LONG_MAX);
			}
		}

		// size returns the number of applicable operators.
		unsigned int size() const {
			return BF;
		}

		// operator[] returns a specific operator.
		Oper operator[] (unsigned int i) const {
			return ops[i];
		}
	};

	struct Edge {
		Cost cost;
		Oper revop;
		Cost revcost;
		State &state;

		Edge(const SynthTree &d, State &s, Oper op) :
		  revop(s.seed), state(s), oldh(s.h), oldd(s.d), oldagd(s.agd) {
			
			double perr = s.err();
		    s.seed = op;
			
		    Rand r(s.seed);
			cost = r.integer(0, MAX_COST);
			revcost = cost;
			Cost n = r.integer(-cost, cost);
			s.agd = std::max(s.agd - n, 0);

			Cost pih = s.agd - perr * s.agd;
			double err = r.real() * d.max_err;
			if(err > perr) {
			  s.h = std::max(pih - 1, 0);
			  s.h = std::min(s.h, (int)(s.agd - d.max_err * s.agd));
			} else {
			  s.h = std::min(pih + 1, s.agd);
			}
			s.d = s.h / MAX_COST + (s.h % MAX_COST != 0);
		}

		~Edge(void) {
			state.h = oldh;
			state.d = oldd;
			state.agd = oldagd;
			state.seed = revop;
		}

	private:
		friend class SynthTree;
		Cost oldh;
		Cost oldd;
		Cost oldagd;
	};

	// Pack the state into the destination packed state.
	// If PackedState is the same type as State then this
	// should at least copy.
	void pack(PackedState &dst, State &src) const {
		dst = src;
	}

	// Unpack the state and return a reference to the
	// resulting unpacked state.  If PackedState and
	// State are the same type then the packed state
	// can just be immediately returned and used
	// so that there is no need to copy.
	State &unpack(State &buf, PackedState &pkd) const {
		return pkd;
	}

	// Print the state.
	void dumpstate(FILE *out, const State &s) const {
		fatal("Unimplemented");
	}

	Cost pathcost(const std::vector<State>&, const std::vector<Oper>&);

private:
    float max_err;
    long seed;
};
