#!/bin/sh
# Builds gnuplot for the PC from the same tarball, patches and gfos.trm the
# stove builds, with GFOS_TERM and tests/gfos_host.c in place of the
# window (a PPM picture and a log), then checks the gfos terminal with
# gnuplot's own plotting: it is listed and the default, its pictures
# (size, colours, which way up, fills, pm3d), size and title options,
# -persist, the script Mathomatic's plot command writes, help text, and a
# run of gnuplot's demos through it.
#   WORK=... sh tests/host-gnuplot.sh
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
. "$HERE/gnuplot.lock"
. "$HERE/scripts/gp-common.sh"
WORK="${WORK:-$HERE/build-host}"; CC="${HOST_CC:-cc}"; JOBS=${JOBS:-8}
mkdir -p "$WORK"; WORK=$(cd "$WORK" && pwd)
SRC="$WORK/gnuplot"
gp_source "$WORK/dl" "$SRC"

# the terminal's PC backend
L="$WORK/gfos"; rm -rf "$L"; mkdir -p "$L"
$CC -O2 -Wall -I"$HERE/amiga" -c "$HERE/amiga/gfos_core.c" -o "$L/gfos_core.o"
$CC -O2 -Wall -I"$HERE/amiga" -c "$HERE/tests/gfos_host.c" -o "$L/gfos_host.o"
ar rcs "$L/libgfos_host.a" "$L/gfos_core.o" "$L/gfos_host.o"

B="$WORK/build"
if [ "$(cat "$B/.src" 2>/dev/null)" != "$(cat "$SRC/.patched")" ]; then
    rm -rf "$B"; mkdir -p "$B"
    # shellcheck disable=SC2086
    (cd "$B" && "$SRC/configure" $GP_CONFIGURE CC="$CC" CPPFLAGS="-DGFOS_TERM -I$HERE/amiga" \
        LIBS="$L/libgfos_host.a" > configure.log 2>&1) || { tail -30 "$B/configure.log"; exit 1; }
    cat "$SRC/.patched" > "$B/.src"
fi
(cd "$B/src" && make -j"$JOBS" gnuplot > make.log 2>&1) || { grep -E 'error' "$B/src/make.log" | head -30; exit 1; }
(cd "$B/docs" && make gnuplot.gih > make.log 2>&1) || { tail -20 "$B/docs/make.log"; exit 1; }
G="$B/src/gnuplot"; ST="python3 $HERE/tests/ppm_stats.py"
T="$WORK/run"; rm -rf "$T"; mkdir -p "$T"
fail=0; n=0
check() { n=$((n + 1)); if eval "$2"; then :; else echo "FAIL: $1"; fail=1; fi; }
gp() { g=$1; shift; (cd "$T" && env -u GNUTERM GFOS_PPM="$T/$g.ppm" GFOS_LOG="$T/$g.log" GNUHELP="$B/docs/gnuplot.gih" timeout 120 "$G" "$@" 2> "$T/$g.err" < /dev/null); }
run() { name=$1; shift; gp "$name" "$@" > /dev/null 2>&1 || { echo "gnuplot failed: $name"; cat "$T/$name.err"; fail=1; }; }

# listed, and the default terminal
echo 'set terminal' | env -u GNUTERM "$G" > "$T/terms.txt" 2>&1 || true
check "gfos is listed" "grep -q '^ *gfos .*GFOS' '$T/terms.txt'"
echo 'show terminal' | env -u GNUTERM "$G" > "$T/show.txt" 2>&1 || true
check "gfos is the default terminal" "grep -q 'terminal type is gfos' '$T/show.txt'"

# a line plot: size, the first line colour, axes, text
run sin -e "set term gfos size 320,240; plot sin(x)"
check "picture size" "[ \"\$($ST $T/sin.ppm size)\" = '320 240' ]"
check "window opened 320 240" "grep -qx 'open 320 240 gnuplot' $T/sin.log"
check "sin(x) in the first line colour" "[ \$($ST $T/sin.ppm count 9400d3) -gt 300 ]"
check "white background" "[ \$($ST $T/sin.ppm count ffffff) -gt 50000 ]"
check "black border" "[ \$($ST $T/sin.ppm count 000000) -gt 600 ]"
check "key text" "grep -q '^text .* sin(x)\$' $T/sin.log"
check "tick labels" "grep -q '^text .* -10\$' $T/sin.log && grep -q '^text .* 10\$' $T/sin.log"

# which way up: a line at y=0.9 lies in the top fifth
run up -e "set term gfos size 200,200; unset border; unset tics; unset key; set margins 0,0,0,0; set xrange [0:1]; set yrange [0:1]; plot 0.9"
check "y counts up" "rows=\$($ST $T/up.ppm rows 9400d3); [ \"\${rows% *}\" -ge 15 ] && [ \"\${rows#* }\" -le 25 ]"

# fills: solid boxes, and a faded one
run box -e "set term gfos size 200,200; unset key; set style fill solid 1.0; plot [0:4] [0:2] '+' using 1:(1.5):(0.8) with boxes"
check "solid boxes fill" "[ \$($ST $T/box.ppm count 9400d3) -gt 3000 ]"
check "boxes are polygons" "grep -q '^polygon 4' $T/box.log"
run fade -e "set term gfos size 200,200; unset key; set style fill solid 0.5; plot [0:4] [0:2] '+' using 1:(1.5):(0.8) with boxes lc rgb '#0000ff'"
check "density 0.5 fades blue" "[ \$($ST $T/fade.ppm count 8080ff) -gt 3000 ]"

# pm3d: a surface in many palette colours
run pm3d -e "set term gfos size 320,240; set pm3d; set isosamples 30; splot x*y with pm3d"
check "pm3d gives many colours" "[ \$($ST $T/pm3d.ppm colours) -gt 100 ]"

# options and -persist
run title -e "set term gfos title 'Mathomatic'; plot x"
check "title option" "grep -qx 'open 640 480 Mathomatic' $T/title.log"
check "options shown back" "echo 'set term gfos size 400,300 title \"T\"; show term' | env -u GNUTERM '$G' 2>&1 | grep -q 'size 400,300 title \"T\"'"
run persist -persist -e "plot x"
check "-persist waits for the window" "[ \"\$(grep -E '^(wait|close)' $T/persist.log | tr '\n' ' ')\" = 'wait close ' ]"
run nopersist -e "plot x"
check "no -persist, no wait" "! grep -q '^wait' $T/nopersist.log"
check "a bad size is refused" "! echo 'set term gfos size 10,10' | env -u GNUTERM '$G' 2>/dev/null; echo 'set term gfos size 10,10' | env -u GNUTERM '$G' 2>&1 | grep -q 'size from 64,48'"

# the script Mathomatic's plot command writes (gfamigamathomatic)
# (as Mathomatic 16.0.5 writes it for y=x^2+1: see gfamigamathomatic's tests)
printf '%s\n' 'set grid; set xlabel "X"; set ylabel "Y";' 'plot  (x**2.0) + 1.0' > "$T/mathomatic.plt"
run matho -persist "$T/mathomatic.plt"
check "Mathomatic's script plots" "[ \$($ST $T/matho.ppm count 9400d3) -gt 300 ] && grep -q '^text .* X\$' $T/matho.log"

# help
check "help gfos" "echo 'help gfos' | env -u GNUTERM GNUHELP='$B/docs/gnuplot.gih' PAGER=cat '$G' 2>&1 | grep -q 'draws in a window on the GFOS'"

# gnuplot's own demos, through gfos (pauses read an empty input)
d=0
for dem in simple surface1 pm3dcolors fillcrvs fillbetween histograms contours multiplt polar steps; do
    if (cd "$SRC/demo" && env -u GNUTERM GFOS_PPM="$T/demo.ppm" GFOS_LOG="$T/demo.log" timeout 120 "$G" "$dem.dem" < /dev/null > /dev/null 2> "$T/$dem.err"); then
        d=$((d + 1))
    else echo "FAIL: demo $dem"; tail -3 "$T/$dem.err"; fail=1; fi
done
n=$((n + 1))
echo "demos through gfos: $d of 10 ran ($(grep -c '^flush' "$T/demo.log") plots)"
[ $fail = 0 ] && echo "host tests: all $n checks passed"
exit $fail
