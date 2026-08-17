# Makefile.am for LuminariMUD utilities

# Custom binary directory - install to project bin
bindir = $(top_builddir)/bin

# These are utility programs, installed to ../bin
bin_PROGRAMS = \
	asciipasswd \
	autowiz \
	plrtoascii \
	rebuildAsciiIndex \
	rebuildMailIndex \
	rol_mob_calculator \
	shopconv \
	sign \
	split \
	wld2html

# Common includes
AM_CFLAGS = -I../src @CSTD_CFLAGS@ @MYFLAGS@

# Source files for each utility
asciipasswd_SOURCES = asciipasswd.c
autowiz_SOURCES = autowiz.c
plrtoascii_SOURCES = plrtoascii.c
rebuildAsciiIndex_SOURCES = rebuildAsciiIndex.c
rebuildMailIndex_SOURCES = rebuildMailIndex.c
rol_mob_calculator_SOURCES = rol_mob_calculator.c ../src/mob/mob_autoroll.c \
	../src/mob/mob_autoroll_profile.c
shopconv_SOURCES = shopconv.c
sign_SOURCES = sign.c
split_SOURCES = split.c
wld2html_SOURCES = wld2html.c

# Libraries needed by some utilities
asciipasswd_LDADD = @CRYPTLIB@

# Don't build in source directory
AUTOMAKE_OPTIONS = subdir-objects

# After installation, remove the executables from the source directory
install-exec-hook:
	rm -f $(bin_PROGRAMS)

# Clean target
clean-local:
	rm -f *.o $(bin_PROGRAMS)
