# To make a universal mac build add: -arch ppc -arch i686 to flags

# You may have to change the next line to where boost is installed.
# For now, you can try adding it to the end, it seems having multiple 
# directories does not hurt.

-include Makefile.includes
ifndef SETUP_INCLUDED
$(error ./configure.sh has not been executed! *** )
endif

FLAGS = -DWATCHEDLITERALS
LINKFLAGS = 
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

ifdef BOOST
  NAMEBASE := $(NAMEBASE)-boost
  FLAGS := $(FLAGS) $(BOOSTINCLUDE) -DUSE_BOOST
  ifdef DYNAMIC
    NAMEBASE := $(NAMEBASE)-dynamic
    LINKFLAGS := $(LINKFLAGS) -lz -lbz2
  else
    LINKFLAGS := $(LINKFLAGS) external_deps/zlib-1.2.3/libz.a external_deps/bzip2-1.0.5/libbz2.a
    BOOSTINCLUDE := $(BOOSTINCLUDE) -Iexternal_deps/zlib-1.2.3/ -Iexternal_deps/bzip2-1.0.5/
  endif
endif
OUTDIR=bin

# Only use our choice of name if one was not provided
ifndef NAME
  NAME = $(NAMEBASE)
endif

OBJDIR=$(OUTDIR)/objdir-$(NAME)

SRC=$(wildcard minion/*.cpp) $(wildcard minion/build_constraints/*.cpp minion/system/*.cpp minion/boost_files/*.cpp)
EXE=bin/$(NAME)


#If you want to optimise for a particular CPU, enable one of these lines
CPU=
#CPU=-march=pentium4
#CPU=-march=pentium-m 
#CPU=-march=pentium-m -mdynamic-no-pic 
# above line seems good for intel macs



FULLFLAGS=-Wextra -Wno-sign-compare $(DEBUG_FLAGS) $(FLAGS) $(CPU) $(MYFLAGS)

OBJFILES=$(patsubst minion/%.cpp,$(OBJDIR)/%.o,$(SRC))

minion: depend mkdirectory external $(OBJFILES)	
	$(CXX) $(FULLFLAGS) -o $(EXE) $(OBJFILES) $(LINKFLAGS)
	
external: 
	cd external_deps/ && ./build_external_deps.sh $(MYFLAGS) $(CPU) 
all: minion generate

minion/svn_header.h: .DUMMY
	mini-scripts/get_svn_version.sh minion/svn_header.h
	
help: 
	bash minion/help/genhelp.sh minion/ > minion/help/help.cpp

# This rule is here to ensure if Makefile.dep is missing, it is built so it can be included.	
Makefile.dep:
	mini-scripts/make_depend.sh
	
depend: Makefile.dep minion/svn_header.h

	
$(OBJDIR)/minion.o : minion/svn_header.h

$(OBJDIR)/%.o: minion/%.cpp 
	$(CXX) $(FULLFLAGS) -c -o $@ $<


	
mkdirectory:
	if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi
	if [ ! -d $(OBJDIR)/build_constraints ]; then mkdir $(OBJDIR)/build_constraints; fi
	if [ ! -d $(OBJDIR)/help ]; then mkdir $(OBJDIR)/help; fi
	if [ ! -d $(OBJDIR)/system ]; then mkdir $(OBJDIR)/system; fi
	if [ ! -d $(OBJDIR)/boost_files ]; then mkdir $(OBJDIR)/boost_files; fi
	

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
	cd external_deps/bzip2-1.0.5/ && make clean
	cd external_deps/zlib-1.2.3/ && make clean

veryclean: clean
	rm -rf bin/*

Makefile.includes:
	echo Please run /configure.sh
	exit
.DUMMY:

include Makefile.dep
