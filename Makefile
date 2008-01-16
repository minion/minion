# To make a universal mac build add: -arch ppc -arch i686 to flags

ifdef DEBUG
 ifdef PRINT
   DEBUG_FLAGS = -D_GLIBCXX_DEBUG -g -DMORE_SEARCH_INFO
   FLAGS =
 else
   DEBUG_FLAGS = -D_GLIBCXX_DEBUG -DNO_PRINT -g -DMORE_SEARCH_INFO
   FLAGS = 
 endif

else
 DEBUG_FLAGS = -DNO_DEBUG
 ifdef PROFILE
   FLAGS = -O2 -g -fno-inline -fno-inline-functions
 else
   FLAGS = -O2
 endif
endif

ifdef INFO
  MORE_INFO_FLAGS = -DMORE_SEARCH_INFO
else
  MORE_INFO_FLAGS =
endif

ifdef NOWATCHED
  WATCHED=
else
  WATCHED=-DWATCHEDLITERALS
endif

ifdef QUICK
  QUICK_COMPILE=-DQUICK_COMPILE
else
  QUICK_COMPILE= 
endif


OUTDIR=bin

ifndef NAME
  NAME=minion
  ifdef DEBUG
    NAME:=$(NAME)-debug
  endif
  ifdef QUICK
    NAME:=$(NAME)-quick
  endif
  ifdef PROFILE
    NAME:=$(NAME)-profile
  endif
  ifdef INFO
    NAME:=$(NAME)-info
  endif
endif

OBJDIR=$(OUTDIR)/objdir-$(NAME)

SRC=$(wildcard minion/*.cpp) $(wildcard minion/build_constraints/*.cpp)
EXE=bin/$(NAME)


#If you want to optimise for a particular CPU, enable one of these lines
CPU=
#CPU=-march=pentium4
#CPU=-march=pentium-m

FULLFLAGS=-W -Wno-sign-compare $(DEBUG_FLAGS) $(FLAGS) $(CPU) $(WATCHED) $(QUICK_COMPILE) $(MORE_INFO_FLAGS) $(MYFLAGS)

OBJFILES=$(patsubst minion/%.cpp,$(OBJDIR)/%.o,$(SRC))

all: svn_version minion generate

svn_version:
	./get_svn_version.sh minion/svn_header.h

$(OBJDIR)/%.o: minion/%.cpp
	$(CXX) $(FULLFLAGS) -c -o $@ $<


	
minion: svn_version mkdirectory $(OBJFILES)
	
	$(CXX) $(FULLFLAGS) -o $(EXE) $(OBJFILES)
	
mkdirectory:
	if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi
	if [ ! -d $(OBJDIR)/build_constraints ]; then mkdir $(OBJDIR)/build_constraints; fi

generate: bibd golomb solitaire steelmill sports

bibd:
	g++ generators/Bibd/MinionBIBDInstanceGenerator.cpp -O2 -o bin/bibd
golomb:
	g++ generators/Golomb/GolombMinionGenerator.cpp -O2 -o bin/golomb
solitaire:
	g++ generators/Solitaire/solitaire-solver.cpp -O2 -o bin/solitaire
steelmill:
	g++ generators/Steelmill/steelmill-solver.cpp -O2 -o  bin/steelmill
sports:
	g++ generators/SportsSchedule/MinionSportsInstanceGenerator.cpp -O2 -o bin/sports

lisp-generate: minion-helper minion-sat minion-quasigroup

minion-helper: 
	clisp -x "(clisp-make-executable \"bin/minion-helper\")" -i generators/MinionHelper.lsp
minion-sat: 
	clisp -C -x "(clisp-make-executable \"bin/minion-sat\" (function clisp-toplevel-sat))" -i generators/MinionHelper.lsp -i generators/SAT/MinionDimacsSAT.lsp  
minion-quasigroup: 
	clisp -C -x "(clisp-make-executable \"bin/minion-quasigroup\" (function clisp-toplevel-quasigroup))" -i generators/MinionHelper.lsp -i generators/Quasigroup/MinionQuasigroup.lsp  

clean:
	rm -rf bin/minion bin/minion-* bin/objdir-minion* bin/bibd bin/golomb bin/solitaire bin/steelmill bin/sports

veryclean:
	rm -rf bin/*

include Makefile.dep
