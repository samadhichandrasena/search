#include "blocksworld.hpp"
#include "../search/main.hpp"
#include <cstdio>

int main(int argc, const char *argv[]) {
	dfheader(stdout);
	Blocksworld d(stdin);
    search<Blocksworld>(d, argc, argv);
	dffooter(stdout);
	return 0;
}
