// Copyright © 2020 the Search Authors under the MIT license. See AUTHORS for the list of authors.                                                             
#pragma once                                                                    
#include "../search/search.hpp"                                                 
#include "../utils/pool.hpp"
#include <limits.h>
#include <iostream>

template <class D> struct MonotonicBeadSearch : public SearchAlgorithm<D> {

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
		Cost f, g, fd, gd;

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
		    if (a->fd == b->fd) {
			    if(a->f == b->f) {
				    return a->g > b->g;
				} else {
				    return a->f < b->f;
				}
			}
			else {
			    return a->fd < b->fd;
			}
		}

		/* Priority of node. */
		static Cost prio(Node *n) {
			return n->fd;
		}

		/* Priority for tie breaking. */
		static Cost tieprio(Node *n) {
			return n->f;
		}

    private:
		ClosedEntry<Node, D> closedent;
    
	};

	MonotonicBeadSearch(int argc, const char *argv[]) :
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
		}

		if (width < 1)
			fatal("Must specify a >0 beam width using -width");
    
		nodes = new Pool<Node>();
	}

	~MonotonicBeadSearch() {
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
		beam[0] = open.pop();
		int used = 1;

		depth = 0;
		sol_count = 0;
		bool done = false;
		int emptied = 0;

		dfrowhdr(stdout, "incumbent", 7, "num", "nodes expanded",
			"nodes generated", "solution depth", "solution cost", "width seen",
			"wall time");

		/* Beam is established, open is empty at start of each iteration.
		   Interleave expanding node from beam and selecting from open.
		 */
		while (!done && !SearchAlgorithm<D>::limit()) {
			depth++;
			int c = 0;
			int i = 0;
			Node *n;
			int first_filled = width;
			Cost f_min = Cost(-1);
			
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
				if(beam[i] && i < first_filled) {
				  first_filled = i;
				}
				if(beam[i]) {
				  if(f_min == -1 || beam[i]->f <= f_min) {
					f_min = beam[i]->f;
				  }
				}
			  }
			  c++;
			  i++;
			}

			while(i < width && !open.empty()) {
			  n = open.pop();
			  n->width_seen = i;
			  beam[i] = dedup(d, n);
			  if(beam[i]) {
				if(f_min == -1 || beam[i]->f <= f_min) {
				  f_min = beam[i]->f;
				}
				i++;
			  }
			}

			used = i;
		    dump_and_clear(d, beam, c, depth);

			if(first_filled == width || used == 0) {
			  done = true;
			}

			if(cand && f_min >= cand->g) {
			  solpath<D, Node>(d, cand, this->res);
			  done = true;
			}

			emptied += first_filled;
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
		kid->gd = parent->gd + Cost(1);
		kid->width_seen = parent->width_seen;
		d.pack(kid->state, e.state);

	    kid->f = kid->g + d.h(e.state);
	    kid->fd = kid->gd + d.d(e.state);
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
		} /*else if(cand && cand->g <= kid->f) {
		  nodes->destruct(kid);
		  return;
		  }*/
		
		open.push(kid);
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->state, s0);
		n0->g = Cost(0);
		n0->gd = Cost(0);
		n0->f = d.h(s0);
		n0->fd = d.d(s0);
		n0->pop = n0->op = D::Nop;
		n0->parent = NULL;
		cand = NULL;
		return n0;
	}

    int width;
    bool dropdups;
    bool dump;
	OpenList<Node, Node, Cost> open;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	Node *cand;
	int depth;
	int sol_count;
  
};
