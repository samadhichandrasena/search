// Copyright © 2020 the Search Authors under the MIT license. See AUTHORS for the list of authors.                                                             
#pragma once                                                                    
#include "../search/search.hpp"                                                 
#include "../utils/pool.hpp"
#include <list>
                                                                                
template <class D> struct TriangleBeadSearch : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;                                
	typedef typename D::Cost Cost;                                              
	typedef typename D::Oper Oper;

	struct Node {
		int openind;
		Node *parent;
		PackedState state;
		Oper op, pop;
		int d;
		Cost f, g;

		Node() : openind(-1) {
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
			if (a->d == b->d)
				return a->g > b->g;
			return a->d < b->d;
		}

		/* Priority of node. */
		static Cost prio(Node *n) {
			return n->d;
		}

		/* Priority for tie breaking. */
		static Cost tieprio(Node *n) {
			return n->g;
		}

    private:
		ClosedEntry<Node, D> closedent;
    
	};

	TriangleBeadSearch(int argc, const char *argv[]) :
		SearchAlgorithm<D>(argc, argv), closed(30000001) {
		dropdups = false;
		for (int i = 0; i < argc; i++) {
			if (strcmp(argv[i], "-dropdups") == 0)
				dropdups = true;
		}
    
		nodes = new Pool<Node>();
	}

	~TriangleBeadSearch() {
		delete nodes;
	}

	Node *dedup(D &d, Node *n) {

	  if(cand && n->f >= cand->f) {
		return NULL;
	  }
	  
	  unsigned long hash = n->state.hash(&d);
	  Node *dup = closed.find(n->state, hash);
	  if(!dup) {
		closed.add(n, hash);
	  } else {
		SearchAlgorithm<D>::res.dups++;
		if(!dropdups && n->g < dup->g) {
		  SearchAlgorithm<D>::res.reopnd++;
					
		  dup->f = dup->f - dup->g + n->g;
		  dup->g = n->g;
		  dup->d = n->d;
		  dup->parent = n->parent;
		  dup->op = n->op;
		  dup->pop = n->pop;
		} else {
		  return NULL;
		}
	  }

	  return n;
	}

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		Node *n0 = init(d, s0);
		closed.add(n0);

		open = new OpenList<Node, Node, Cost>();

		openlists.push_front(open);

		expand(d, n0, s0);

		depth = 0;
		sol_count = 0;

		dfrowhdr(stdout, "incumbent", 7, "num", "nodes expanded",
			"nodes generated", "solution depth", "solution cost", "width seen",
			"wall time");

		bool done = false;
		int n_open = 0;
    
		while (!done && !SearchAlgorithm<D>::limit()) {
		  depth++;
		  done = true;
		  n_open = 0;

		  auto open_it = openlists.end();
		  open_it--;
		  open = *open_it;
			
		  bool looped = false;
			
		  while(!looped) {
			Node *n = NULL;
				  
			while(!n && !open->empty()) {
			  n = dedup(d, open->pop());
			}

			n_open += !open->empty();

			if(open_it != openlists.begin()) {
			  open = *(--open_it);
			} else {
			  open = new OpenList<Node, Node, Cost>();
			  openlists.push_front(open);
			  looped = true;
			}

			if(!n) {
			  if(done) {
				//fprintf(stdout, "pruning an open list.\n");
				openlists.pop_back();
			  }
			  continue;
			}
			/*if(cand)
			  fprintf(stdout, "current candidate has f = %f\n", cand->f);
			fprintf(stdout, "expanding node with f = %f\n", n->f);
			fprintf(stdout, "%lu nodes have been expanded\n", SearchAlgorithm<D>::res.expd);*/
				  
			State buf, &state = d.unpack(buf, n->state);
			expand(d, n, state);
			
			done = false;
		  }
		  //fprintf(stdout, "%d non-empty open lists\n", n_open);
		  //fprintf(stdout, "%lu total open lists\n", openlists.size());
		}

		if(cand) {
		  solpath<D, Node>(d, cand, this->res);
		  done = true;
		}
		this->finish();
	}

	virtual void reset() {
		SearchAlgorithm<D>::reset();
		open->clear();
		closed.clear();
		delete nodes;
		nodes = new Pool<Node>();
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open list type", "%s", open->kind());
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
		assert (kid);
		typename D::Edge e(d, state, op);
		kid->g = parent->g + e.cost;
		d.pack(kid->state, e.state);

		kid->f = kid->g + d.h(e.state);
		kid->d = d.d(e.state);
		kid->parent = parent;
		kid->op = op;
		kid->pop = e.revop;
		
		State buf, &kstate = d.unpack(buf, kid->state);
		if (d.isgoal(kstate) && (!cand || kid->g < cand->g)) {
		  cand = kid;
		  sol_count++;
		  dfrow(stdout, "incumbent", "uuuugug", sol_count, this->res.expd,
			this->res.gend, 0, cand->g, 0,
			walltime() - this->res.wallstart);
		  return;
		} else if(cand && cand->g <= kid->f) {
		  nodes->destruct(kid);
		  return;
		}
		
		open->push(kid);
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->state, s0);
		n0->d = 0;
		n0->g = Cost(0);
		n0->f = d.h(s0);
		n0->pop = n0->op = D::Nop;
		n0->parent = NULL;
		cand = NULL;
		return n0;
	}

    bool dropdups;
 	std::list<OpenList<Node, Node, Cost> *> openlists;
 	OpenList<Node, Node, Cost> *open;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	Node *cand;
	int width;
	int depth;
	int sol_count;
  
};
