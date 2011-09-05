#include "../incl/search.hpp"
#include "../structs/htable.hpp"
#include "openlist.hpp"
#include <boost/pool/object_pool.hpp>
#include <boost/optional.hpp>

template <class D> class Astar : public Search<D> {
public:

	Result<D> search(D &d, typename D::State &s0) {
		res = Result<D>(true);

		Node *n0 = init(d, s0);
		closed.add(n0);
		open.push(n0);

		while (!open.empty()) {
			Node *n = *open.pop();

			if (d.isgoal(n->state)) {
				handlesol(n);
				break;
			}

			expand(d, n);
		}

		res.finish();
		closed.prstats(stdout, "closed ");

		return res;
	}

	Astar(void) : closed(30000001) {}

private:

	typedef typename D::State State;
	typedef typename D::Undo Undo;
	typedef typename D::Cost Cost;
	typedef typename D::Oper Oper;

	struct Node {
		State state;
		Cost g, f;
		Oper pop;
		Node *htblnxt, *parent;
		int openind;
	};

	void expand(D &d, Node *n) {
		res.expd++;
		for (unsigned int i = 0; i < d.nops(n->state); i++) {
			Oper op = d.nthop(n->state, i);
			if (op == n->pop)
				continue;
			Node *k = kid(d, n, op);
			res.gend++;
			considerkid(d, k);
		}
	}

	void considerkid(D &d, Node *k) {
		unsigned long h = k->state.hash();
		Node *dup = closed.find(&k->state, h);
		if (dup) {
			res.dups++;
			if (k->g >= dup->g) {
				nodes.free(k);
				return;
			}
			if (!open.mem(dup)) {
				open.push(dup);
			} else {
				open.pre_update(dup);
				dup->f = dup->f - dup->g + k->g;
				dup->g = k->g;
				open.post_update(dup);
			}
			nodes.free(k);
		} else {
			closed.add(k, h);
			open.push(k);
		}
	}

	Node *kid(D &d, Node *parent, Oper op) {
		Node *kid = nodes.malloc();
		d.applyinto(kid->state, parent->state, op);
		kid->g = parent->g + d.opcost(parent->state, op);
		kid->f = kid->g + d.h(kid->state);
		kid->pop = d.revop(parent->state, op);
		kid->parent = parent;
		kid->openind = OpenList<Openops, Node*, Cost>::InitInd;
		return kid;
	}

	Node *init(D &d, State &s0) {
		Node *n0 = nodes.malloc();
		n0->state = s0;
		n0->g = 0;
		n0->f = d.h(s0);
		n0->pop = D::Nop;
		n0->parent = NULL;
		n0->openind = OpenList<Openops, Node*, Cost>::InitInd;
		return n0;
	}

	void handlesol(Node *n) {
		res.cost = n->g;

		for ( ; n; n = n->parent)
			res.path.push_back(n->state);
	}

	struct Openops {
		static void setind(Node *n, int i) {n->openind = i;}
		static int getind(Node *n) {return n->openind;}
		static Cost prio(Node *n) {return n->f;}

		static bool pred(Node *a, Node *b) {
			if (a->f == b->f)
				return a->g > b->g;
			return a->f < b->f;
		}
	};

	struct Closedops {
		static State *key(Node *n) { return &n->state; }
		static unsigned long hash(State *s) { return s->hash(); }
		static bool eq(State *a, State *b) { return a->eq(*b); }
		static Node **nxt(Node *n) { return &n->htblnxt; }
	};

	Result<D> res;
	OpenList<Openops, Node*, Cost> open;
 	Htable<Closedops, State*, Node> closed;
	boost::object_pool<Node> nodes;
};