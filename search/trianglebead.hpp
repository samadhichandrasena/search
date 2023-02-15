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

	struct RingNode {
	  RingNode *next;
	  RingNode *prev;
	  OpenList<Node, Node, Cost> *list;
	  RingNode(OpenList<Node, Node, Cost> *l) {
		next = NULL;
		prev = NULL;
		list = l;
	  }
	  ~RingNode() {
		delete list;
	  }
	};
  
	struct Ring {
	  RingNode *begin;
	  RingNode *end;
	  int maxsize;
	  int size;
	  int removed;
	  int reused;

	  Ring() {
		begin = new RingNode(NULL);
		end = new RingNode(NULL);
		begin->next = end;
		begin->prev = end;
		end->next = begin;
		end->prev = begin;
		maxsize = 0;
		size = 0;
		removed = 0;
		reused = 0;
	  }

	  ~Ring() {
		while(begin->next != end) {
		  RingNode *temp = begin;
		  begin = begin->next;
		  delete temp;
		}
		delete begin;
		delete end;
	  }

	  void move_after(RingNode *n, RingNode *b) {
		n->prev->next = n->next;
		n->next->prev = n->prev;
		b->next->prev = n;
		n->next = b->next;
		n->prev = b;
		b->next = n;
	  }

	  void add() {
		if(begin->prev == end) {
		  RingNode *n = new RingNode(new OpenList<Node, Node, Cost>());
		  n->next = begin->next;
		  n->prev = begin;
		  begin->next->prev = n;
		  begin->next = n;
		  maxsize++;
		  size++;
		} else {
		  move_after(begin->prev, begin);
		  size++;
		  reused++;
		}
	  }

	  void remove_rest(RingNode *from) {
		RingNode *a = from->prev;
		RingNode *b = begin->next;

		begin->next = from;
		from->prev = begin;
		a->next = begin;
		b->prev = begin->prev;
		begin->prev->next = b;
		begin->prev = a;
	  }

	  void remove() {
	    move_after(end->prev, end);
		size--;
		removed++;
	  }
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

		openlists.add();
		auto open_it = openlists.end->prev;
		open = open_it->list;
		auto last_filled = open_it;

		open_count = 0;

		expand(d, n0, s0);

		sol_count = 0;
		depth = 1;

		dfrowhdr(stdout, "incumbent", 5, "num", "nodes expanded",
			"nodes generated", "solution cost", "wall time");

		bool done = false;
    
		while (!done && !SearchAlgorithm<D>::limit()) {
		  done = true;
		  depth++;

		  open_it = openlists.end->prev;
		  open = open_it->list;
			
		  bool looped = false;
			
		  while(!looped) {
			Node *n = NULL;
				  
			while(!n && !open->empty()) {
			  n = dedup(d, open->pop());
			  open_count--;
			}

			if(open_it->prev != openlists.begin) {
			  open_it = open_it->prev;
			  open = open_it->list;
			} else {
			  openlists.add();
			  open_it = openlists.begin->next;
			  open = open_it->list;
			  looped = true;
			}

			if(n)
			  last_filled = open_it;

			if(!n) {
			  if(done) {
				openlists.remove();
			  }
			  continue;
			}
				  
			State buf, &state = d.unpack(buf, n->state);
			expand(d, n, state);
		    
			
			done = false;
		  }

		  if(last_filled != openlists.begin->next) {
			openlists.remove_rest(last_filled);
		  }
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
		dfpair(stdout, "open lists created", "%d", openlists.maxsize);
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
		  dfrow(stdout, "incumbent", "uuugg", sol_count, this->res.expd,
				this->res.gend, cand->g,
				walltime() - this->res.wallstart);
		  return;
		} else if(cand && cand->g <= kid->f) {
		  nodes->destruct(kid);
		  return;
		}

		open_count++;
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
    Ring openlists;
 	OpenList<Node, Node, Cost> *open;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	Node *cand;
	int width;
	int depth;
	int open_count;
	int sol_count;
  
};
