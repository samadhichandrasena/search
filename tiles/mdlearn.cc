#include "mdlearn.hpp"
#include <cstdlib>
#include <cstring>
#include <boost/array.hpp>
#include <algorithm>

TilesMDLearn::TilesMDLearn(FILE *in) : TilesMdist(in) {
	initops(10);
}

void TilesMDLearn::initops(unsigned int dmax) {
	unsigned int oldsz = ops.size();
	if (dmax+1 > ops.capacity())
		ops.reserve((dmax+1) * 2);
	ops.resize(dmax+1);
	for (unsigned int d = 0; d < oldsz; d++)
		initdests(d);
	for (unsigned int d = oldsz; d <= dmax; d++) {
		for (unsigned int i = 0; i < Ntiles; i++) {
			memset(&ops[d][i], 0, sizeof(ops[d][i]));
			if (i >= Width)
				ops[d][i].mvs[ops[d][i].n++].b = i - Width;
			if (i % Width > 0)
				ops[d][i].mvs[ops[d][i].n++].b = i - 1;
			if (i % Width < Width - 1)
				ops[d][i].mvs[ops[d][i].n++].b = i + 1;
			if (i < Ntiles - Width)
				ops[d][i].mvs[ops[d][i].n++].b = i + Width;
		}
		initdests(d);
	}
}

void TilesMDLearn::initdests(unsigned int d) {
	for (unsigned int i = 0; i < Ntiles; i++) {
	for (unsigned int j = 0; j < ops[d][i].n; j++) {
		Pos b = ops[d][i].mvs[j].b;
		ops[d][i].dests[b] = &ops[d][i].mvs[j];
	}
	}
}

void TilesMDLearn::resetprobs(void) {
	for (unsigned int d = 0; d < ops.size(); d++) {
	for (unsigned int i = 0; i < Ntiles; i++) {
	for (unsigned int j = 0; j < ops[d][i].n; j++) {
		ops[d][i].mvs[j].nother = 0;
		ops[d][i].mvs[j].ndec = 0;
	}
	}
	}
}

void TilesMDLearn::sortops(void) {
	for (unsigned int d = 0; d < ops.size(); d++) {
		for (unsigned int i = 0; i < Ntiles; i++)
			std::sort(ops[d][i].mvs.begin(), ops[d][i].mvs.begin() + ops[d][i].n);
		initdests(d);
	}
}