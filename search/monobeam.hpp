// Copyright © 2020 the Search Authors under the MIT license. See AUTHORS for the list of authors.                                                             
#pragma once                                                                    
#include "../search/search.hpp"                                                 
#include "../utils/pool.hpp"

template <class D> struct MonotonicBeamSearch : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;                                
	typedef typename D::Cost Cost;                                              
	typedef typename D::Oper Oper;

	struct Node {
		int openind;
		int width_seen;
		Node *parent;
		PackedState state;
		Oper op, pop;
		Cost f, g;

	  Node() : openind(-1), width_seen(INT_MAX) {
		}

		static ClosedEntry<Node, D> &closedentry(Node *n) {
			return n->closedent;
		}

		static PackedState &key(Node *n) {
			return n->state;
		}

		/* Set index of node on open list. */
		static void setind(Node *n, int i) {
			n->openind = i;
		}

		/* Get index of node on open list. */
		static int getind(const Node *n) {
			return n->openind;
		}

		/* Indicates whether Node a has better value than Node b. */
		static bool pred(Node *a, Node *b) {
			if (a->f == b->f)
				return a->g > b->g;
			return a->f < b->f;
		}

		/* Priority of node. */
		static Cost prio(Node *n) {
			return n->f;
		}

		/* Priority for tie breaking. */
		static Cost tieprio(Node *n) {
			return n->g;
		}

    private:
		ClosedEntry<Node, D> closedent;
    
	};

	MonotonicBeamSearch(int argc, const char *argv[]) :
		SearchAlgorithm<D>(argc, argv), closed(30000001) {
		dropdups = false;
		for (int i = 0; i < argc; i++) {
			if (i < argc - 1 && strcmp(argv[i], "-width") == 0)
				width = atoi(argv[++i]);
			if (strcmp(argv[i], "-dropdups") == 0)
				dropdups = true;
		}

		if (width < 1)
			fatal("Must specify a >0 beam width using -width");
    
		nodes = new Pool<Node>();
	}

	~MonotonicBeamSearch() {
		delete nodes;
	}

	Node *dedup(D &d, Node *n) {
	  unsigned long hash = n->state.hash(&d);
	  Node *dup = closed.find(n->state, hash);
	  if(!dup) {
		closed.add(n, hash);
	  } else if(n->width_seen < dup->width_seen) {
		SearchAlgorithm<D>::res.dups++;
		SearchAlgorithm<D>::res.reopnd++;
		dup->width_seen = n->width_seen;
		if(n->g < dup->g) {
		  dup->f = dup->f - dup->g + n->g;
		  dup->g = n->g;
		  dup->parent = n->parent;
		  dup->op = n->op;
		  dup->pop = n->pop;
		}
	  } else {
		return NULL;
	  }

	  return n;
	}
  
	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		Node *n0 = init(d, s0);
		n0->width_seen = 0;
		closed.add(n0);
		open.push(n0);

		Node **beam = new Node*[width];
		beam[0] = open.pop();
		int used = 1;

		int depth = 0;
		bool done = false;

		/* Beam is established, open is empty at start of each iteration.
		   Interleave expanding node from beam and selecting from open.
		 */
		while (!done && !SearchAlgorithm<D>::limit()) {
			depth++;
			int c = 0;
			int i = 0;
			Node *n;
			while(c < used && i < width
				  && !done && !SearchAlgorithm<D>::limit()) {
			  
 			  n = beam[c];
			  if(n) {
				State buf, &state = d.unpack(buf, n->state);
				
				expand(d, n, state);
			  }

			  beam[i] = NULL;
			  while(!open.empty() && !beam[i]) {
				n = open.pop();
				n->width_seen = i;
			    beam[i] = dedup(d, n);
			  }
			  c++;
			  i++;
			}

			if(done)
			  break;

			while(i < width && !open.empty()) {
			  n = open.pop();
			  n->width_seen = i;
			  beam[i] = dedup(d, n);
			  if(beam[i]) {
				i++;
			  }
			}

			used = i;
			open.clear();

			bool empty = true;

		    
			for(int x = 0; x < used; x++) {
			  if(beam[x])
				empty = false;
			}

			if(empty) {
			  done = true;
			}

			if(cand) {
			  solpath<D, Node>(d, cand, this->res);
			  done = true;
			}
		}

		delete[] beam;
		this->finish();
	}

	virtual void reset() {
		SearchAlgorithm<D>::reset();
		open.clear();
		closed.clear();
		delete nodes;
		nodes = new Pool<Node>();
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open list type", "%s", open.kind());
		dfpair(stdout, "node size", "%u", sizeof(Node));
	}


private:

	void expand(D &d, Node *n, State &state) {
		SearchAlgorithm<D>::res.expd++;

		typename D::Operators ops(d, state);
		for (unsigned int i = 0; i < ops.size(); i++) {
			if (ops[i] == n->pop)
				continue;
			SearchAlgorithm<D>::res.gend++;
			considerkid(d, n, state, ops[i]);
		}
	}

	void considerkid(D &d, Node *parent, State &state, Oper op) {
		Node *kid = nodes->construct();
		assert(kid);
		typename D::Edge e(d, state, op);
		kid->g = parent->g + e.cost;
		kid->width_seen = parent->width_seen;
		d.pack(kid->state, e.state);
		State buf, &kstate = d.unpack(buf, kid->state);
		if (d.isgoal(kstate) && (!cand || kid->g < cand->g)) {
		  cand = kid;
		}

	    kid->f = kid->g + d.h(e.state);
		kid->parent = parent;
		kid->op = op;
		kid->pop = e.revop;
		
		open.push(kid);
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->state, s0);
		n0->g = Cost(0);
		n0->f = d.h(s0);
		n0->pop = n0->op = D::Nop;
		n0->parent = NULL;
		cand = NULL;
		return n0;
	}

    int width;
    bool dropdups;
	OpenList<Node, Node, Cost> open;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	Node *cand;
  
};
