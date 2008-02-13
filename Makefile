# To make a universal mac build add: -arch ppc -arch i686 to flags

FLAGS = -DWATCHEDLITERALS
NAMEBASE = minion

ifdef DEBUG
 NAMEBASE := $(NAMEBASE)-debug
 ifdef PRINT
   DEBUG_FLAGS = -D_GLIBCXX_DEBUG -g -DMORE_SEARCH_INFO
 else
   DEBUG_FLAGS = -D_GLIBCXX_DEBUG -DNO_PRINT -g -DMORE_SEARCH_INFO
 endif
else
  FLAGS := $(FLAGS) -O3 -DNO_DEBUG
endif

ifdef PROFILE
  NAMEBASE := $(NAMEBASE)-profile
  FLAGS := $(FLAGS) -g -fno-inline -fno-inline-functions
endif

ifdef INFO
  FLAGS := $(FLAGS) -DMORE_SEARCH_INFO
  NAMEBASE := $(NAMEBASE)-info
endif

ifdef QUICK
  FLAGS := $(FLAGS) -DQUICK_COMPILE
  NAMEBASE := $(NAMEBASE)-quick
endif

ifdef REENTER
  FLAGS := $(FLAGS) -DREENTER
  NAMEBASE := $(NAMEBASE)-reenter
endif

OUTDIR=bin

# Only use our choice of name if one was not provided
ifndef NAME
  NAME = $(NAMEBASE)
endif

OBJDIR=$(OUTDIR)/objdir-$(NAME)

SRC=$(wildcard minion/*.cpp) $(wildcard minion/build_constraints/*.cpp minion/system/*.cpp) minion/help/help.cpp
EXE=bin/$(NAME)


#If you want to optimise for a particular CPU, enable one of these lines
CPU=
#CPU=-march=pentium4
#CPU=-march=pentium-m 
#CPU=-march=pentium-m -mdynamic-no-pic 
# above line seems good for intel macs



FULLFLAGS=-Wextra -Wno-sign-compare $(DEBUG_FLAGS) $(FLAGS) $(CPU) $(MYFLAGS)

OBJFILES=$(patsubst minion/%.cpp,$(OBJDIR)/%.o,$(SRC))

all: minion generate

minion/svn_header.h: .DUMMY
	mini-scripts/get_svn_version.sh minion/svn_header.h
	
minion/help/help.cpp: .DUMMY
	bash minion/help/genhelp.sh minion/ > minion/help/help.cpp
	
$(OBJDIR)/minion.o : minion/svn_header.h

$(OBJDIR)/%.o: minion/%.cpp 
	$(CXX) $(FULLFLAGS) -c -o $@ $<

minion: depend mkdirectory $(OBJFILES)	
	$(CXX) $(FULLFLAGS) -o $(EXE) $(OBJFILES)
	
mkdirectory:
	if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi
	if [ ! -d $(OBJDIR)/build_constraints ]; then mkdir $(OBJDIR)/build_constraints; fi
	if [ ! -d $(OBJDIR)/help ]; then mkdir $(OBJDIR)/help; fi
	if [ ! -d $(OBJDIR)/system ]; then mkdir $(OBJDIR)/system; fi

generate: bibd golomb solitaire steelmill sports

bibd:
	g++ generators/Bibd/MinionBIBDInstanceGenerator.cpp -O2 -o bin/bibd $(FULLFLAGS)
golomb:
	g++ generators/Golomb/GolombMinionGenerator.cpp -O2 -o bin/golomb $(FULLFLAGS)
solitaire:
	g++ generators/Solitaire/solitaire-solver.cpp -O2 -o bin/solitaire $(FULLFLAGS)
steelmill:
	g++ generators/Steelmill/steelmill-solver.cpp -O2 -o  bin/steelmill $(FULLFLAGS)
sports:
	g++ generators/SportsSchedule/MinionSportsInstanceGenerator.cpp -O2 -o bin/sports $(FULLFLAGS)


htmlhelp:
	bash docs/genhelp/genhelp.sh minion

lisp-generate: minion-helper minion-sat minion-quasigroup

minion-helper: 
	clisp -x "(clisp-make-executable \"bin/minion-helper\")" -i generators/MinionHelper.lsp
minion-sat: 
	clisp -C -x "(clisp-make-executable \"bin/minion-sat\" (function clisp-toplevel-sat))" -i generators/MinionHelper.lsp -i generators/SAT/MinionDimacsSAT.lsp  
minion-quasigroup: 
	clisp -C -x "(clisp-make-executable \"bin/minion-quasigroup\" (function clisp-toplevel-quasigroup))" -i generators/MinionHelper.lsp -i generators/Quasigroup/MinionQuasigroup.lsp  

objclean:
	rm -rf bin/objdir-*

clean:
	rm -rf bin/minion bin/minion-* bin/objdir-minion* bin/bibd bin/golomb bin/solitaire bin/steelmill bin/sports

veryclean:
	rm -rf bin/*

# Make sure these things get constructed before doing a make depend.
depend: minion/svn_header.h
	mini-scripts/make_depend.sh

.DUMMY:

include Makefile.dep
