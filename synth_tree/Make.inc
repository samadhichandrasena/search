synth_tree:\
	synth_tree/synth_tree_solver\

SYNTHTREEOBJS=\
    synth_tree/synth_tree.o

include $(SYNTHTREEOBJS:.o=.d)
CLEAN+=$(SYNTHTREEOBJS)

EVERYTHING+=synth_tree

synth_tree/synth_tree_solver:\
	$(SYNTHTREEOBJS)\
	synth_tree/main.o\
	utils/utils.a\
	structs/structs.a\
	search/search.a
	@echo $@
	@$(CXX) $^ $(LDFLAGS) -o $@

include synth_tree/main.d
CLEAN+=synth_tree/synth_tree_solver synth_tree/main.o