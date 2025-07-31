#!/bin/sh
#
# Debugging Script for Valingrad
#
valgrind --leak-check=yes bin/circle -q 4001
