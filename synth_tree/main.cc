// © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.

#include "synth_tree.hpp"
#include "../search/main.hpp"
#include <cstdio>

int main(int argc, const char *argv[]) {
	dfheader(stdout);
	
	SynthTree d(stdin, cost);
	search<SynthTree>(d, argc, argv);
	dffooter(stdout);
	return 0;
}
