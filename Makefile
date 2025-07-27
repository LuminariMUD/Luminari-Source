# LuminariMUD
# Generated automatically from Makefile.in by configure.
# tbaMUD Makefile.in - Makefile template used by 'configure'
# Clean-up provided by seqwith.

# C compiler to use
CC = gcc

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-aggressive-loop-optimizations -Wno-unused-value --param=max-vartrack-size=60000000
#MYFLAGS = -Wno-error=format-truncation -Werror -Wall -Wwrite-strings -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-value --param=max-vartrack-size=60000000
#MYFLAGS = -Wall

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################
MKTIME	:= \""$(shell date)"\"
MKUSER  := \""$(USER)"\"
MKHOST  := \""$(HOSTNAME)"\"
BRANCH	:= \""$(shell git branch)"\"
PARENT	:= \""$(shell git rev-parse HEAD)"\"

BINDIR = ../bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE) 
CXXFLAGS = $(CFLAGS) -std=c++11

LIBS =  -lstdc++ -lcrypt -lgd -lm -lmysqlclient -lcurl

SRCFILES := $(wildcard *.c)
CPPFILES := $(wildcard *.cpp)
OBJFILES := $(patsubst %.c,%.o,$(SRCFILES)) $(CPPFILES:%.cpp=%.o)

default: circle

all:
	$(MAKE) $(BINDIR)/circle
	$(MAKE) utils

utils:
	(cd util; $(MAKE) all)

circle:
	$(MAKE) $(BINDIR)/circle

$(BINDIR)/circle : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)

# Always rebuild constants.c with other files so that luminari_build is updated
constants.c: $(filter-out constants.c,$(SRCFILES))
	touch constants.c

constants.o: CFLAGS += -DMKTIME=$(MKTIME) -DMKUSER=$(MKUSER) -DMKHOST=$(MKHOST) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)

SRCSCUTEST := $(filter-out unittests/CuTest/AllTests.c,$(wildcard unittests/CuTest/*.c)) unittests/CuTest/CuTest.c
OBJSCUTEST := $(SRCSCUTEST:%.c=%.o) $(OBJFILES)

unittests/CuTest/AllTests.c: $(SRCSCUTEST)
	unittests/CuTest/make-tests.sh unittests/CuTest/*.c > $@

.PHONY: cutest
cutest: $(BINDIR)/cutest
$(BINDIR)/cutest: CFLAGS += -DLUMINARI_CUTEST
$(BINDIR)/cutest: $(OBJSCUTEST) unittests/CuTest/AllTests.o
	$(CC) -o $@ $(PROFILE) $^ $(LIBS) && $@

clean:
	rm -f *.o depend

# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend

-include depend

