# Makefile.am for LuminariMUD utilities

# These are utility programs, not installed by default
noinst_PROGRAMS = \
	asciipasswd \
	autowiz \
	hl_events \
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
hl_events_SOURCES = hl_events.c
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