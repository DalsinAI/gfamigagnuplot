#!/bin/sh
# PC test for scripts/write-part.sh: the Part file GFOpenUpTool reads.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/C" "$T/Docs/gnuplot" "$T/Share/gnuplot/PostScript"
: > "$T/C/gnuplot"; : > "$T/Docs/gnuplot/gnuplot.gih"; : > "$T/Share/gnuplot/gnuplotrc"; : > "$T/Share/gnuplot/PostScript/prologue.ps"
sh "$HERE/../scripts/write-part.sh" "$T" 6.0.5
fail=0
expect() { grep -qxF "$1" "$T/Part" || { echo "FAIL: no line: $1"; fail=1; }; }
expect 'part gnuplot 6.0.5'
expect 'needs 68040'
expect 'copy "C/gnuplot" "SYS:C/gnuplot"'
expect 'copy "Docs/gnuplot/gnuplot.gih" "SYS:Docs/gnuplot/gnuplot.gih"'
expect 'copy "Share/gnuplot/gnuplotrc" "SYS:Share/gnuplot/gnuplotrc"'
expect 'copy "Share/gnuplot/PostScript/prologue.ps" "SYS:Share/gnuplot/PostScript/prologue.ps"'
[ "$(head -1 "$T/Part")" = 'part gnuplot 6.0.5' ] || { echo "FAIL: first line"; fail=1; }
[ "$(grep -c '^copy' "$T/Part")" = 4 ] || { echo "FAIL: file count (the Part file itself must not be listed)"; fail=1; }
sh "$HERE/../scripts/write-part.sh" "$T" 6.0.5   # a second run: the same file
[ "$(grep -c '^copy' "$T/Part")" = 4 ] || { echo "FAIL: second run lists the Part file"; fail=1; }
[ $fail = 0 ] && echo "Part file: all checks passed"
exit $fail
