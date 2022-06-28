// Copyright © 2020 the Search Authors under the MIT license. See AUTHORS for the list of authors.                                                             
#pragma once                                                                    
#include "../search/search.hpp"                                                 
#include "../utils/pool.hpp"
                                                                                
template <class D> struct ParallelHillClimbingD : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;                                
	typedef typename D::Cost Cost;                                              
	typedef typename D::Oper Oper;

	struct Node {
		int openind;
		Node *parent;
		PackedState state;
		Oper op, pop;
		Cost f, g, fd, gd;

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
			if (a->fd == b->fd)
				return a->f < b->f;
			return a->fd < b->fd;
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

	ParallelHillClimbingD(int argc, const char *argv[]) :
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

	~ParallelHillClimbingD() {
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

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		int depth = 0;

		bool done = false;
	    Node **beam = new Node*[width];

		for(int i = 0; i < width; i++)
		  beam[i] = NULL;

		Node *n0 = init(d, s0);
		beam[0] = n0;
    
		while (!done && !SearchAlgorithm<D>::limit()) {
			depth++;

			int c = 0;
			for(int i = 0; i < width && !SearchAlgorithm<D>::limit(); i++) {
			    Node *n = beam[i];
				if(!n) {
				  break;
				}

				Node *bc = NULL;
				
				SearchAlgorithm<D>::res.expd++;

				State buf, &state = d.unpack(buf, n->state);
				typename D::Operators ops(d, state);
				for (unsigned int i = 0; i < ops.size(); i++) {
				  if (ops[i] == n->pop)
					continue;
				  SearchAlgorithm<D>::res.gend++;
				  considerkid(d, n, state, ops[i], bc);
				}

				beam[c] = bc;
				if(bc) {
				  closed.add(bc);
				  c++;
				}
			}

			if(c == 0) {
			  done = true;
			}
      
			for(int i = c; !done && !open.empty() && i < width; i++) {
			  beam[i] = NULL;

			  beam[i] = open.pop();
			  closed.add(beam[i]);
			}

			dump_and_clear(d, beam, c, depth);

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

  void considerkid(D &d, Node *parent, State &state, Oper op, Node *&bc) {
		Node *kid = nodes->construct();
		assert (kid);
		typename D::Edge e(d, state, op);
		kid->g = parent->g + e.cost;
		kid->gd = parent->gd + Cost(1);
		d.pack(kid->state, e.state);

		kid->f = kid->g + d.h(e.state);
		kid->fd = kid->gd + d.d(e.state);
		kid->parent = parent;
		kid->op = op;
		kid->pop = e.revop;

		unsigned long hash = kid->state.hash(&d);
		Node *dup = closed.find(kid->state, hash);
		if(dup) {
		  SearchAlgorithm<D>::res.dups++;
		  if(!dropdups && kid->g < dup->g) {
			SearchAlgorithm<D>::res.reopnd++;
					
			dup->f = dup->f - dup->g + kid->g;
			dup->fd = dup->fd - dup->gd + kid->gd;
			dup->g = kid->g;
			dup->gd = kid->gd;
			dup->parent = kid->parent;
			dup->op = kid->op;
			dup->pop = kid->pop;
		  } else {
			nodes->destruct(kid);
			return;
		  }
		}
		
		State buf, &kstate = d.unpack(buf, kid->state);
		if (d.isgoal(kstate) && (!cand || kid->g < cand->g)) {
		  cand = kid;
		  return;
		}

		if(!bc || Node::pred(kid, bc)) {
		  if(bc)
			open.push(bc);
		  bc = kid;
		} else {
		  open.push(kid);
		}
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
  
};
