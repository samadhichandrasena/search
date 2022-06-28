// Copyright © 2022 the Search Authors under the MIT license. See AUTHORS for the list of authors.                                                             
#pragma once                                                                    
#include "../search/search.hpp"                                                 
#include "../utils/pool.hpp"
#include <limits.h>
#include <iostream>

template <class D> struct MonoFloorSearch : public SearchAlgorithm<D> {

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

	MonoFloorSearch(int argc, const char *argv[]) :
		SearchAlgorithm<D>(argc, argv), closed(30000001) {
		dropdups = false;
		dump = false;
		for (int i = 0; i < argc; i++) {
			if (i < argc - 1 && strcmp(argv[i], "-width") == 0)
				width = atoi(argv[++i]);
			if (strcmp(argv[i], "-dropdups") == 0)
				dropdups = true;
			if (strcmp(argv[i], "-dump") == 0)
				dump = true;
			if (strcmp(argv[i], "-n") == 0)
				slot_n = atoi(argv[++i]);
		}

		if (width < 1)
			fatal("Must specify a >0 beam width using -width");
		if (slot_n < 1)
			fatal("Must specify n > 0 using -n");
    
		nodes = new Pool<Node>();
	}

	~MonoFloorSearch() {
		delete nodes;
	}
  
    void dump_and_clear(D &d, Node **beam, int c, int depth) {
	    if(dump) { 
		  Node *tmp;
		  fprintf(stderr, "depth: %d\n", depth);
		  fprintf(stderr, "used states:\n");
		  for(int i = 0; i < c; i++) {
			tmp = beam[i];
			State buf, &state = d.unpack(buf, tmp->state);
			d.dumpstate(stderr, state);
			fprintf(stderr, "\n");
			double node_g = tmp->g;
			fprintf(stderr, "g: %f\n", node_g);
			fprintf(stderr, "\n");
			double node_h = d.h(state);
			fprintf(stderr, "h: %f\n", node_h);
			fprintf(stderr, "\n");
			double node_d = d.d(state);
			fprintf(stderr, "d: %f\n", node_d);
			fprintf(stderr, "\n");
		  }
		  
		  fprintf(stderr, "unused states:\n");
		  
		  while(!open.empty()) {
			tmp = open.pop();
			State buf, &state = d.unpack(buf, tmp->state);
			d.dumpstate(stderr, state);
			fprintf(stderr, "\n");
			
			nodes->destruct(tmp);
		  }
		} else {
		  while(!open.empty()) {
			nodes->destruct(open.pop());
		  }
		  //open.clear();
		}
	}

  Node *dedup(D &d, Node *n) {
	  unsigned long hash = n->state.hash(&d);
	  Node *dup = closed.find(n->state, hash);
	  if(!dup) {
		closed.add(n, hash);
	  } else {
		SearchAlgorithm<D>::res.dups++;
		if(n->width_seen < dup->width_seen) {
		  // never true in second part of beam
		  closed.remove(dup->state, hash);
		  closed.add(n, hash);
		} else {
		  if(dropdups || n->g >= dup->g) {
			nodes->destruct(n);
			return NULL;
		  } else {
			SearchAlgorithm<D>::res.reopnd++;
			if(n->width_seen == dup->width_seen && n->g < dup->g) {
			  closed.remove(dup->state, hash);
			  closed.add(n, hash);
			} 
		  }
		}
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
		for (int i = 0; i < width; i++) {
		  beam[i] = NULL;
		}
		beam[0] = open.pop();

		depth = 0;
		sol_count = 0;
		bool done = false;

		dfrowhdr(stdout, "incumbent", 7, "num", "nodes expanded",
			"nodes generated", "solution depth", "solution cost", "width seen",
			"wall time");

		/* Beam is established, open is empty at start of each iteration.
		   Interleave expanding node from beam and selecting from open for 
		   the first part of the beam, then expand all remaining nodes and 
		   select to fill the next part of the beam.
		 */
		while (!done && !SearchAlgorithm<D>::limit()) {
			depth++;
			int c = 0;
			int i = 0;
			Node *n;
			double f_sum = 0;
			int count = 0;
			bool first_part;
			
			while(c < width && i < width
				  && !done && !SearchAlgorithm<D>::limit()) {
			  
 			  n = beam[c];
			  beam[c] = NULL;
			  c++;

			  first_part = i < width - slot_n;
			  
			  if(n) {
				State buf, &state = d.unpack(buf, n->state);
				
				expand(d, n, state);
			  }

			  beam[i] = NULL;

			  if (!first_part) {
				continue;
			  }
			  
			  while(!open.empty() && !beam[i]) {
				n = open.pop();
				n->width_seen = i;
			    beam[i] = dedup(d, n);
			  }
			  if(beam[i]) {
				f_sum += n->f;
				count++;
			  }
			  i++;
			}

			while(i < width && !open.empty()) {
			  first_part = i < width - slot_n;
			  n = open.pop();
			  n->width_seen = first_part ? i : width - slot_n;
			  beam[i] = dedup(d, n);
			  if(beam[i]) {
				i++;
				f_sum += n->f;
				count++;
			  }
			}

			dump_and_clear(d, beam, c, depth);

			if(count == 0) {
			  done = true;
			} else {
			  //dfpair(stdout, "depth", "%d", depth);
			  //dfpair(stdout, "avg f value", "%f", f_sum/count);
			}

			if(cand && cand->width_seen == 0) {
			  solpath<D, Node>(d, cand, this->res);
			  done = true;
			}
		}
		
		if(cand) {
		  solpath<D, Node>(d, cand, this->res);
		}

		dfpair(stdout, "final depth", "%d", depth);

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

	    kid->f = kid->g + d.h(e.state);
		kid->parent = parent;
		kid->op = op;
		kid->pop = e.revop;

		
		State buf, &kstate = d.unpack(buf, kid->state);
		if (d.isgoal(kstate) && (!cand || kid->g < cand->g)) {
		  cand = kid;
		  sol_count++;
		  dfrow(stdout, "incumbent", "uuuugug", sol_count, this->res.expd,
				this->res.gend, depth, cand->g, cand->width_seen,
			walltime() - this->res.wallstart);
		  return;
		} else if(cand && cand->g <= kid->f) {
		  nodes->destruct(kid);
		  return;
		}
		
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
    int slot_n;
    bool dropdups;
    bool dump;
	OpenList<Node, Node, Cost> open;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	Node *cand;
	int depth;
	int sol_count;
  
};
