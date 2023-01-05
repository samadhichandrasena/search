// Copyright © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.
#include "../search/search.hpp"
#include "../utils/pool.hpp"


template <class D> struct MinTest : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;
	typedef typename D::Cost Cost;
	typedef typename D::Oper Oper;

	struct Node {
		ClosedEntry<Node, D> closedent;
		int openind;
		int mindepth;
		Node *parent;
		PackedState state;
		Oper op, pop;
		Cost h, hwm;

		Node() : openind(-1) {
		}

		static ClosedEntry<Node, D> &closedentry(Node *n) {
			return n->closedent;
		}

		static PackedState &key(Node *n) {
			return n->state;
		}

		static void setind(Node *n, int i) {
			n->openind = i;
		}

		static int getind(const Node *n) {
			return n->openind;
		}
	
		static bool pred(Node *a, Node *b) {
          if(a->mindepth == b->mindepth) {
			return a->h < b->h;
          } else {
			return a->mindepth > b->mindepth;
          }
		}

		static typename D::Cost prio(Node *n) {
			fprintf(stdout, "prio called!\n");
			return -1 * n->mindepth;
		}

		static typename D::Cost tieprio(Node *n) {
			fprintf(stdout, "tieprio called!\n");
			return -1 * n->h;
		}
	};

	MinTest(int argc, const char *argv[]) :
		SearchAlgorithm<D>(argc, argv), closed(30000001) {
		nodes = new Pool<Node>();
		dump = false;
        for (int i = 0; i < argc; i++) {
		  if (strcmp(argv[i], "-dump") == 0)
			dump = true;
        }
	}

	~MinTest() {
		delete nodes;
	}

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		Node *n0 = init(d, s0);
		closed.add(n0);
		open.push(n0);

		int minsize = 0;
		int minsum = 0;
		int mincount = 0;
		int minmax = 0;
		hh = (Cost)0;
		Cost min_hwm = (Cost)0;
		int min_exp = 0;
		fprintf(stderr, "Local min sizes:\n");

		while (!open.empty() && !SearchAlgorithm<D>::limit()) {
			Node *n = open.pop();
			State buf, &state = d.unpack(buf, n->state);

			// if h is lower than max seen so far, increment local min
			if(n->h < n->hwm) {
			  if(n->mindepth == 1) {
				if(minsize > 0) {
				  // record current minimum size else
				  fprintf(stderr, "size: %d hwm: %f expanded: %d\n", minsize, (double)min_hwm, min_exp);
				  mincount++;
				  minsum += minsize;
				  if(minsize > minmax) {
					minmax = minsize;
				  }
				  // reset min size
				  minsize = 0;
				}
				min_hwm = hh;
				min_exp = SearchAlgorithm<D>::res.expd;
			  }
			  min_hwm = hh;
			  minsize++;
			}
			// if h is higher than max, reset min size to zero and update max
			else if(n->h > hh) {
			  hh = n->h;
			}

			if(dump && n) {
			  fprintf(stderr, "expanded state:\n");
			  d.dumpstate(stderr, state);
			}

			expand(d, n, state);
		}

		if(minsize > 0) {
		  // record current minimum size else
		  fprintf(stderr, "size: %d hwm: %f expanded: %d\n", minsize, (double)min_hwm, min_exp);
		  mincount++;
		  minsum += minsize;
		  if(minsize > minmax) {
			minmax = minsize;
		  }
		  // reset min size
		  minsize = 0;
		}

		fprintf(stderr, "local min count: %d\n", mincount);
		fprintf(stderr, "max local min size: %d\n", minmax);
		fprintf(stderr, "mean local min size: %0.2f\n",
				(double)minsum / mincount);
		fprintf(stderr, "total nodes in local minima: %d\n", minsum);
		
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
		typename D::Edge e(d, state, op);
		kid->h = d.h(e.state);
		kid->hwm = std::max(hh, kid->h);
		d.pack(kid->state, e.state);
		if(kid->h < hh) {
		  kid->mindepth = parent->mindepth + 1;
		} else {
		  kid->mindepth = 0;
		}

		unsigned long hash = kid->state.hash(&d);
		Node *dup = closed.find(kid->state, hash);
        
		if (dup) {
			this->res.dups++;
            bool isopen = open.mem(dup);
			if(isopen) {
              open.pre_update(dup);
			  dup->mindepth = std::max(dup->mindepth, kid->mindepth);
			  open.post_update(dup);
			}
			nodes->destruct(kid);
		} else {
			kid->parent = parent;
			kid->op = op;
			kid->pop = e.revop;
			closed.add(kid, hash);
			open.push(kid);
        }
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->state, s0);
		n0->mindepth = 0;
		n0->h = d.h(s0);
		n0->hwm = Cost(0);
		n0->op = n0->pop = D::Nop;
		n0->parent = NULL;
		return n0;
	}

    bool dump;
    Cost hh;
	OpenList<Node, Node, Cost> open;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
};
