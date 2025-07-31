#!/bin/sh
#
# Multi-Port Update Juggling Script, by Zusuk
#
# this quick script is just for quick updates of the src between codebases
# specifically to transfer the svn

svn update /home/luminari/luminari/src/ -r HEAD --force
cd /home/luminari/luminari/src/
make all -j 5
