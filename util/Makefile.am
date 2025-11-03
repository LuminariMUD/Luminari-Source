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
	shopconv \
	sign \
	split \
	webster \
	wld2html

# Common includes
AM_CFLAGS = -I../src

# Source files for each utility
asciipasswd_SOURCES = asciipasswd.c
autowiz_SOURCES = autowiz.c
plrtoascii_SOURCES = plrtoascii.c
rebuildAsciiIndex_SOURCES = rebuildAsciiIndex.c
rebuildMailIndex_SOURCES = rebuildMailIndex.c
shopconv_SOURCES = shopconv.c
sign_SOURCES = sign.c
split_SOURCES = split.c
webster_SOURCES = webster.c
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