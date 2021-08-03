synth_tree:\
	synth_tree/synth_tree_solver

CLEAN+=\
	synth_tree/synth_tree_solver

EVERYTHING+=synth_tree

SYNTHTREESRC=\
	synth_tree/synth_tree.cc\

CLEAN+=$(SYNTHTREESRC:.cc=.o)

synth_tree/synth_tree_solver: synth_tree/main.d
synth_tree/synth_tree_solver:\
	$(SYNTHTREESRC)\
	synth_tree/main.cc\
	utils/utils.a\
	structs/structs.a\
	search/search.a
	@echo $@
	@$(CXX) $^ $(LDFLAGS) -o $@

include synth_tree/main.d
