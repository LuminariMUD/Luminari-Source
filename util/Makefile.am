# Makefile.am for LuminariMUD utilities

# Custom binary directory
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

# Clean target
clean-local:
	rm -f *.o