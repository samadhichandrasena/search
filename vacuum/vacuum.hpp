// Copyright © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.
#pragma once

#include "../utils/utils.hpp"
#include "../gridnav/gridmap.hpp"
#include <memory>
#include <cstring>
#include <cstdio>

class Vacuum {
public:
	typedef float Cost;

	typedef int Oper;	// directions 0, 1, 2, 3
	static const Oper Suck = 4;
	static const Oper Charge = 5;
	static const Oper Nop = -1;
	static const Oper Magic = -2;
	static const Oper MakeGoal = -3;

	Vacuum(FILE*, const char *);

	class State {
	public:
		State() : loc(-1), energy(-1), ndirt(-1), start_dirt(-1), back(false) {
		}

		bool eq(const Vacuum*, const State &o) const {
			if (loc != o.loc || ndirt != o.ndirt || start_dirt != o.start_dirt)
				return false;

			for (unsigned int i = 0; i < dirt->size(); i++) {
				if (dirt->at(i) != o.dirt->at(i))
					return false;
			}

			return true;
		}

		unsigned long hash(const Vacuum*) const {
			return loc*dirt->size() + ndirt;
		}

		int loc, energy, ndirt, weight, start_dirt;
		bool back;
		std::shared_ptr<std::vector<bool> > dirt;
	};

	typedef State PackedState;

	State initialstate(void) const;

	Cost h(const State &s) const {
		if (s.start_dirt != -1)
		  return 0;

		auto pt = map->coord(s.loc);

		int minx = pt.first;
		int maxx = minx;
		int miny = pt.second;
		int maxy = miny;
		
		unsigned int i;
		for (i = 0; i < s.dirt->size() && (s.dirt->at(i) + back) != 1; i++)
			;

		int ndirt = 1;

		for (; i < s.dirt->size(); i++) {
			if ((s.dirt->at(i) + back) != 1)
				continue;
			ndirt++;
			int x = dirtLocs[i].first, y = dirtLocs[i].second;
			if (x < minx)
				minx = x;
			if (x > maxx)
				maxx = x;
			if (y < miny)
				miny = y;
			if (y > maxy)
				maxy = y;
		}

		assert(s.weight > 0);
		return ndirt + ((maxx-minx) + (maxy-miny)) * s.weight;
	}

	Cost d(const State &s) const {
		if (s.start_dirt != -1)
		  return 0;

		auto pt = map->coord(s.loc);

		int minx = pt.first;
		int maxx = minx;
		int miny = pt.second;
		int maxy = miny;
		
		unsigned int i;
		for (i = 0; i < s.dirt->size() && (s.dirt->at(i) + back) != 1; i++)
			;

		int ndirt = 1;

		for (i++; i < s.dirt->size(); i++) {
			if ((s.dirt->at(i) + back) != 1)
				continue;
			ndirt++;
			int x = dirtLocs[i].first, y = dirtLocs[i].second;
			if (x < minx)
				minx = x;
			if (x > maxx)
				maxx = x;
			if (y < miny)
				miny = y;
			if (y > maxy)
				maxy = y;
		}

		return ndirt + (maxx-minx) + (maxy-miny);
	}

	bool isgoal(const State &s) const {
		return s.ndirt == 0;
	}

	struct Operators {
		Operators(const Vacuum &d, const State &s) : n(0) {
			if (s.start_dirt == d.orig_dirt)
				return;
			
			if (s.start_dirt != -1) {
				ops[n++] = MakeGoal;
				ops[n++] = Magic;
				return;
			}
			
			if (s.energy == 0)
				return;

			int dirt = d.dirt[s.loc];
			if (dirt >= 0 && s.dirt->at(dirt))
				ops[n++] = Suck;

			for (unsigned int i = 0; i < d.map->nmvs; i++) {
				if (d.map->ok(s.loc, d.map->mvs[i]))
					ops[n++] = i;
			}

			if (d.chargers[s.loc])
				ops[n++] = Charge;
		}

		unsigned int size() const {
			return n;
		}

		Oper operator[] (unsigned int i) const {
			return ops[i];
		}

	private:
		unsigned int n;
		Oper ops[6];	// Up, Down, Left, Right, Suck, Charge
	};

	struct Edge {
		State state;
		Cost cost;
		Oper revop;
		Cost revcost;

		Edge(const Vacuum &d, const State &s, Oper op) : state(s), cost(s.weight), revcost(1) {
			if (op == MakeGoal) {
				std::pair<int, int> startLoc = d.dirtLocs[s.start_dirt];
				state.loc = d.map->index(startLoc.first, startLoc.second);
				
				int dirt = s.start_dirt;
				state.start_dirt = -1;

				state.dirt = std::make_shared<std::vector<bool>>(s.dirt->begin(), s.dirt->end());
				state.dirt->at(dirt) = false;
				state.ndirt--;
			    state.weight += d.cost_mod;
				cost = 1;
				
				revop = Nop;
				revcost = Cost(-1);
			} else if (op == Magic) {
				state.start_dirt++;
				revop = Nop;
				revcost = Cost(-1);
			} else if (op == Suck) {
				int dirt = d.dirt[s.loc];

				assert (dirt >= 0);
				assert ((unsigned int) dirt < d.ndirt());
				assert (state.dirt->at(dirt));

				state.dirt = std::make_shared<std::vector<bool>>(s.dirt->begin(), s.dirt->end());
				state.dirt->at(dirt) = false;
				state.ndirt--;
			    state.weight += d.cost_mod;
				cost = 1;

				revop = Nop;
				revcost = Cost(-1);

			} else if (op == Charge) {
				fatal("Charge operator!");

			} else {
				assert (op >= 0);
				assert (op <= 3);
				revop = d.rev[op];
				state.loc += d.map->mvs[op].delta;
			}
		}

		~Edge(void) { }
	};

	void pack(PackedState &dst, State &src) const {
		dst = src;
	}

	State &unpack(State &buf, PackedState &pkd) const {
		return pkd;
	}

	void dumpstate(FILE *out, const State &s) const {
		auto pt = map->coord(s.loc);
		fprintf(out, "(%d, %d), energy=%d, ndirt=%d", pt.first, pt.second, s.energy, s.ndirt);
		for (unsigned int i = 0; i < s.dirt->size(); i++)
			fprintf(out, " %d", (int) s.dirt->at(i));
		fprintf(out, "\n");
	}

	Cost pathcost(const std::vector<State>&, const std::vector<Oper>&);

	void printpath(FILE*, const std::vector<Oper>&) const;

protected:
	int orig_dirt;
	Cost cost_mod;
	int orig_cost;
	int start_dirt;
	bool back;

private:

	unsigned int ndirt() const {
		return dirtLocs.size();
	}

	// reverseops computes the reverse operators
	void reverseops();

	// rev holds the index of the reverse of each operator
	int rev[8];

	GridMap *map;
	int x0, y0;

	std::vector<int> dirt;	// Indexed by map location, gives dirt ID.
	std::vector<std::pair<int, int> > dirtLocs;
	std::vector<bool> chargers;
};
