// © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.

#include "pancake.hpp"
#include "../search/main.hpp"
#include <cstdio>

int main(int argc, const char *argv[]) {
	dfheader(stdout);

	const char *cost = "unit";
	for (int i = 0; i < argc; i++) {
		if(i < argc - 1 && strcmp(argv[i], "-cost") == 0)
			cost = argv[++i];
	}
	dfpair(stdout, "cost", "%s", cost);
	
	Pancake d(stdin, cost);
	search<Pancake>(d, argc, argv);
	dffooter(stdout);
	return 0;
}
