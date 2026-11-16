#!/bin/sh
# Writes the Part file GFOpenUpTool INSTALL reads (gfopenup src/openuptool.c)
# for the part drawer the build made: C/gnuplot to C:gnuplot, Docs/gnuplot/
# to SYS:Docs/gnuplot/ (with gnuplot.gih, the help text) and Share/gnuplot/
# to SYS:Share/gnuplot/ (gnuplotrc, colour sets, PostScript prologues).
#   write-part.sh <part drawer> <version>
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
P=$1; V=$2
{
    echo "part gnuplot $V"
    echo "; gnuplot: the plotting program (C:gnuplot), drawing in GFOS windows (set terminal gfos)"
    echo "needs 68040"
    (cd "$P" && find C Docs Share -type f | LC_ALL=C sort) | while read -r f; do
        echo "copy \"$f\" \"SYS:$f\""
    done
} > "$P/Part"
