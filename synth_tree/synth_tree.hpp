#include "../utils/utils.hpp"
#include <cstdio>

struct SynthTree {

	typedef int Cost;

	// The type of an operator which can be
	// applied to a state.  This is usually just an
	// integer but it may be some more complex
	// class.  Searches assume that operator==
	// is defined on the Oper class.
	typedef int Oper;
	static const int Nop = -1;

	SynthTree(FILE*);

	struct State {
	public:
        

	private:
		Cost h;
		Cost d;
	};

    typedef State PackedState;

	unsigned long hash(const PackedState&) const {
		return -1;
	}

	// Get the initial state.
	State initialstate(void) const;

	// Get the heuristic.
	Cost h(const State &s) const {
		fatal("Unimplemented");
		return 0.0;
	}

	// Get a distance estimate.
	Cost d(const State &s) const {
		fatal("Unimplemented");
		return 0.0;
	}

	// Is the given state a goal state?
	bool isgoal(const State &s) const {
		return s.h == 0;
	}

	// Operators implements an vector of the applicable
	// operators for a given state.
	struct Operators {
		Operators(const SynthTree&, const State&);

		// size returns the number of applicable operatiors.
		unsigned int size() const {
			fatal("Unimplemented");
			return 0;
		}

		// operator[] returns a specific operator.
		Oper operator[] (unsigned int) const { 
			fatal("Unimplemented");
			return 0;
		}
	};

	struct Edge {
		Cost cost;
		Oper revop;
		Cost revcost;

		// The state field may or may not be a reference.
		// The reference variant is used in domains that
		// do in-place modification and the non-reference
		// variant is used in domains that do out-of-place
		// modification.
		State state;
		// State &state

		// Applys the operator to thet state.  Some domains
		// may modify the input state in this constructor.
		// Because of this, a search algorithm may not
		// use the state passed to this constructor until
		// after the Edge's destructor has been called!
		Edge(const SynthTree&, const State&, Oper) { }

		// The destructor is expected to undo any changes
		// that it may have made to the input state in
		// the constructor.  If a domain uses out-of-place
		// modification then the destructor may not be
		// required.
		~Edge(void) { }
	};

	// Pack the state into the destination packed state.
	// If PackedState is the same type as State then this
	// should at least copy.
	void pack(PackedState &dst, State &src) const {
		fatal("Unimplemented");
	}

	// Unpack the state and return a reference to the
	// resulting unpacked state.  If PackedState and
	// State are the same type then the packed state
	// can just be immediately returned and used
	// so that there is no need to copy.
	State &unpack(State &buf, PackedState &pkd) const {
		fatal("Unimplemented");
		return buf;
	}

	// Print the state.
	void dumpstate(FILE *out, const State &s) const {
		fatal("Unimplemented");
	}

	Cost pathcost(const std::vector<State>&, const std::vector<Oper>&);
};
