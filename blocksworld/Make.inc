blocksworld:\
    blocksworld/20bw_solver\
    blocksworld/50bw_solver\
    blocksworld/20bwdp_solver\
    blocksworld/50bwdp_solver


CLEAN+=\
    blocksworld/20bw_solver\
    blocksworld/50bw_solver\
    blocksworld/20bwdp_solver\
    blocksworld/50bwdp_solver

EVERYTHING+=blocksworld

BWRC=\
	blocksworld/blocksworld.cc\

CLEAN+=$(BWRC:.cc=.o)

blocksworld/%bwdp_solver: blocksworld/main.d
blocksworld/%bwdp_solver:\
	$(BWRC)\
	blocksworld/main.cc\
	utils/utils.a\
	structs/structs.a\
	search/search.a
	@echo $@
	@$(CXX) $(BWRC)\
		blocksworld/main.cc\
		utils/utils.a\
		structs/structs.a\
		search/search.a\
		$(LDFLAGS) -DNBLOCKS=$* -DDEEP -o $@

blocksworld/%bw_solver: blocksworld/main.d
blocksworld/%bw_solver:\
	$(BWRC)\
	blocksworld/main.cc\
	utils/utils.a\
	structs/structs.a\
	search/search.a
	@echo $@
	@$(CXX) $(BWRC)\
		blocksworld/main.cc\
		utils/utils.a\
		structs/structs.a\
		search/search.a\
		$(LDFLAGS) -DNBLOCKS=$* -o $@

include blocksworld/main.d
