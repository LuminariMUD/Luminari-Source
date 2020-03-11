# Generated automatically from Makefile.in by configure.
# tbaMUD Makefile.in - Makefile template used by 'configure'
# Clean-up provided by seqwith.

# C compiler to use
CC = gcc

# Any special flags you want to pass to the compiler
#MYFLAGS = -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-aggressive-loop-optimizations -Wno-unused-value --param=max-vartrack-size=60000000
MYFLAGS = -Wimplicit-int -Werror=implicit-function-declaration # -Wall -Wno-char-subscripts -Wno-unused-but-set-variable -Wno-unused-value --param=max-vartrack-size=60000000
#MYFLAGS = -Wall

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)

LIBS =  -lcrypt -lgd -lm -lmysqlclient

OBJDIR := obj
SRCFILES := $(wildcard *.c)
OBJFILES := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES))
TEST_SRCFILES := $(wildcard unittests/*.c) act.comm.do_spec_comm.c helpers.c
TEST_OBJFILES := $(patsubst %.c,$(OBJDIR)/%.o,$(TEST_SRCFILES))
ALL_SRCFILES := $(TEST_SRCFILES) $(SRCFILES)
ALL_OBJFILES := $(TEST_OBJFILES) $(OBJFILES)

default: all

all: .accepted
	$(MAKE) $(BINDIR)/circle
	$(MAKE) utils

.accepted:
	@./licheck less

utils: .accepted
	(cd util; $(MAKE) all)

circle: test $(BINDIR)/circle

.PHONY: test circle clean $(wildcard unittests/*-Makefile)
test: $(wildcard unittests/*-Makefile)
	for mkf in $(notdir $<); do
	  make -C unittests -f $$mkf
	done

$(BINDIR)/circle: $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)

$(BINDIR)/unittests: $(TEST_OBJFILES)
	$(CC) -o $@ $(PROFILE) $(LIBS) -O0 $^

clean:
	rm -f $(ALL_OBJFILES)
	rm -rf $(DEPDIR)

# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#tldr
DEPDIR := $(OBJDIR)/.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

clean:
	rm -f *.o depend

$(OBJDIR)/unittests/%.o : unittests/%.c
$(OBJDIR)/unittests/%.o : unittests/%.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $(DEPDIR) $(DEPDIR)/unittests $(OBJDIR)/unittests

DEPFILES := $(ALL_SRCFILES:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

-include depend
