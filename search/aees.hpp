// Copyright © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.
#include "../search/search.hpp"
#include "../utils/pool.hpp"

void fatal(const char*, ...);	// utils.hpp

template <class D> struct AnytimeEES : public SearchAlgorithm<D> {

	typedef typename D::State State;
	typedef typename D::PackedState PackedState;
	typedef typename D::Cost Cost;
	typedef typename D::Oper Oper;

	struct Node {

		ClosedEntry<Node, D> closedent;

		// values for tracking location in focal, open, and f-ordered list
		int openind;
		int focalind;
		int fopenind;

		Node *parent;
		PackedState state;
		Oper op, pop;
		Cost f, g, h;
		int d;
		double hhat, fhat, dhat;

		Node() : openind(-1), focalind(-1), fopenind(-1) {
		}

		static ClosedEntry<Node, D> &closedentry(Node *n) {
			return n->closedent;
		}

		static PackedState &key(Node *n) {
			return n->state;
		}
	};

	struct FOps {
		static void setind(Node *n, int i) {
			n->fopenind = i;
		}

		static int getind(const Node *n) {
			return n->fopenind;
		}
	
		static bool pred(Node *a, Node *b) {
			if (a->f == b->f) {
				if (a->d == b->d)
					return a->g > b->g;
				return a->d < b->d;
			}
			return a->f < b->f;
		}	
	};
  
	struct DHatOps {
		static void setind(Node *n, int i) {
			n->focalind = i;
		}

		static int getind(const Node *n) {
			return n->focalind;
		}
	
		static bool pred(Node *a, Node *b) {
			if (a->dhat == b->dhat) {
				if (a->fhat == b->fhat)
					return a->g > b->g;
				return a->fhat < b->fhat;
			}
			return a->dhat < b->dhat;
		}	
	};

	struct FHatOps {
		static void setind(Node *n, int i) {
			n->openind = i;
		}

		static int getind(const Node *n) {
			return n->openind;
		}
	
		static bool pred(Node *a, Node *b) {
			if (a->fhat == b->fhat) {
				if (a->d == b->d)
					return a->g > b->g;
				return a->d < b->d;
			}
			return a->fhat < b->fhat;
		}	
	};

	AnytimeEES(int argc, const char *argv[]) :
			SearchAlgorithm<D>(argc, argv), herror(0), derror(0), 
			dropdups(false), wt(-1.0), closed(30000001) {
		for (int i = 0; i < argc; i++) {
			if (i < argc - 1 && strcmp(argv[i], "-wt0") == 0)
				wt = strtod(argv[++i], NULL);
			if (strcmp(argv[i], "-dropdups") == 0)
				dropdups = true;
		}

		if (wt < 1)
			fatal("Must specify a starting weight ≥1 weight using -wt0");

		nodes = new Pool<Node>();
	}

	~AnytimeEES() {
		delete nodes;
	}

	Node *select_node() {
	  Node *bestDHat = NULL;
	  if(!focal.empty()) {
		bestDHat = *focal.front();
	  }
	  Node *bestFHat = *open.front();
	  Node *bestF = *fopen.front();

	  if(bestDHat && bestDHat->fhat <= wt*bestF->f) {
			focal.pop();
			open.remove(bestDHat);
			fopen.remove(bestDHat);
			return bestDHat;
	  }

	  if(bestFHat->fhat <= wt*bestF->f) {
			open.pop();
			fopen.remove(bestFHat);
			if(bestFHat->focalind >= 0) {
				focal.remove(bestFHat);
			}
			return bestFHat;
	  }

	  fopen.pop();
	  open.remove(bestF);
	  if(bestF->focalind >= 0) {
			focal.remove(bestF);
	  }
	  return bestF;

	}

	void goalfound(Node *n) {
		if(!cand || n->g < cand->g) {
			cand = n;
			sol_count++;
			dfrow(stdout, "incumbent", "uuuggg", sol_count, this->res.expd,
				  this->res.gend, wt, (float)cand->g,
				  walltime() - this->res.wallstart);
			if(fopen.empty()) {
				  wt = 1.0;
			} else {
				  wt = cand->g / (*fopen.front())->f;
			}
		}
	}

	void search(D &d, typename D::State &s0) {
		this->start();
		closed.init(d);

		Node *n0 = init(d, s0);
		closed.add(n0);
		open.push(n0);
		focal.push(n0);
		fopen.push(n0);

		sol_count = 0;
		dfrowhdr(stdout, "incumbent", 6, "num", "nodes expanded",
				 "nodes generated", "solution bound", "solution cost",
				 "wall time");

		while (!open.empty() && !SearchAlgorithm<D>::limit()) {
			Node *n = select_node();
			State buf, &state = d.unpack(buf, n->state);

			if(d.isgoal(state)) {
				goalfound(n);
			}

			expand(d, n, state);
		}

		if(cand) {
			solpath<D, Node>(d, cand, this->res);
		}
		this->finish();
	}

	virtual void reset() {
		SearchAlgorithm<D>::reset();
		open.clear();
		focal.clear();
		fopen.clear();
		closed.clear();
		delete nodes;
		nodes = new Pool<Node>();
		herror = 0;
		derror = 0;
	}

	virtual void output(FILE *out) {
		SearchAlgorithm<D>::output(out);
		closed.prstats(stdout, "closed ");
		dfpair(stdout, "open list type", "%s", "binary heap");
		dfpair(stdout, "node size", "%u", sizeof(Node));
		dfpair(stdout, "weight", "%lg", wt);
		dfpair(out, "h error last", "%g", herror);
		dfpair(out, "d error last", "%g", derror);
	}

private:

	void expand(D &d, Node *n, State &state) {
		if(cand && n->f > cand->f) {
			return;
		}
	  
		SearchAlgorithm<D>::res.expd++;

        double herrnext = 0;
		double derrnext = 0;

        Node *bestkid = NULL;

		typename D::Operators ops(d, state);
		for (unsigned int i = 0; i < ops.size(); i++) {
			if (ops[i] == n->pop)
				continue;
			SearchAlgorithm<D>::res.gend++;

			Node *kid = nodes->construct();
			typename D::Edge e(d, state, ops[i]);
			kid->g = n->g + e.cost;
			typename D::Cost h = d.h(e.state);
			kid->h = h;
			kid->f = kid->g + h;
			kid->d = d.d(e.state);

			if(cand && kid->f >= cand->g) {
				nodes->destruct(kid);
				continue;
			}
			
			d.pack(kid->state, e.state);

			unsigned long hash = kid->state.hash(&d);
			Node *dup = static_cast<Node*>(closed.find(kid->state, hash));
			if (dup) {
				this->res.dups++;
				if (dropdups || kid->g >= dup->g) {
					nodes->destruct(kid);
					continue;
				}
				if (dup->openind < 0)
					this->res.reopnd++;
				dup->f = dup->f - dup->g + kid->g;
				dup->g = kid->g;
				double dhat = dup->d / (1 - derror);
				double hhat = dup->h + (herror * dhat);
				dup->hhat = hhat;
				dup->fhat = dup->g + dup->hhat;
				dup->parent = n;
				dup->op = ops[i];
				dup->pop = e.revop;
				open.pushupdate(dup, dup->openind);
				fopen.pushupdate(dup, dup->fopenind);
				if(dup->fhat <= wt * (*open.front())->fhat) {
					focal.pushupdate(dup, dup->focalind);
				} else if(dup->focalind >= 0) {
					focal.remove(dup);
				}
				nodes->destruct(kid);
 
				if (!bestkid || dup->hhat < bestkid->hhat)
					bestkid = dup;
			} else {
				double dhat = kid->d / (1 - derror);
				double hhat = kid->h + (herror * dhat);
				kid->hhat = hhat;
				kid->fhat = kid->g + kid->hhat;
				kid->parent = n;
				kid->op = ops[i];
				kid->pop = e.revop;
				if(!d.isgoal(state)) {
					closed.add(kid, hash);
				}
				open.push(kid);
				fopen.push(kid);
				if(kid->fhat <= wt * (*open.front())->fhat) {
					focal.push(kid);
				}
 
				if (!bestkid || kid->hhat < bestkid->hhat)
					bestkid = kid;
			}
		}

		if (bestkid) {
		  double herr =  bestkid->f - n->f;
		  if (herr < 0)
		  	herr = 0;
		  double pastErrSum = herror * ((SearchAlgorithm<D>::res.expd)+imExp-1);
		  herrnext = (herr + pastErrSum)/((SearchAlgorithm<D>::res.expd)+imExp);
		  // imagine imExp of expansions with no error
		  // regulates error change in beginning of search

		  double derr = (bestkid->d+1) - n->d;
		  if (derr < 0)
			derr = 0;
		  if (derr >= 1)
			derr = 1 - geom2d::Threshold;
		  double pastDSum = derror * ((SearchAlgorithm<D>::res.expd)+imExp-1);
		  derrnext = (derr + pastDSum)/((SearchAlgorithm<D>::res.expd)+imExp);
		  // imagine imExp of expansions with no error
		  // regulates error change in beginning of search
		  
		  herror = herrnext;
		  derror = derrnext;
		}
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes->construct();
		d.pack(n0->state, s0);
		n0->h = d.h(s0);
		n0->g = Cost(0);
		n0->f = n0->h + n0->g;
		n0->dhat = n0->d;
		n0->hhat = n0->h;
		n0->fhat = n0->f;
		n0->d = d.d(s0);
		n0->op = n0->pop = D::Nop;
		n0->parent = NULL;
		cand = NULL;
		return n0;
	}

	double herror;
	double derror;

	bool dropdups;
	double wt;
	BinHeap<FHatOps, Node*> open;
	BinHeap<DHatOps, Node*> focal;
	BinHeap<FOps, Node*> fopen;
 	ClosedList<Node, Node, D> closed;
	Pool<Node> *nodes;
	Node *cand;
	int sol_count;

    int imExp = 10;
};
