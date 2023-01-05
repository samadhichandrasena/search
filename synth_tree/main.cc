// © 2013 the Search Authors under the MIT license. See AUTHORS for the list of authors.

#include "synth_tree.hpp"
#include "../search/main.hpp"
#include <cstdio>

int main(int argc, const char *argv[]) {
	dfheader(stdout);

	float h_err = 0;
	for (int i = 0; i < argc; i++) {
		if(i < argc - 1 && strcmp(argv[i], "-err") == 0)
			h_err = strtod(argv[++i], NULL);
	}
	dfpair(stdout, "max h error", "%f", h_err);
	
	SynthTree d(stdin, h_err);
	search<SynthTree>(d, argc, argv);
	dffooter(stdout);
	return 0;
}
