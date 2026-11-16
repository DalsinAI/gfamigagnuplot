#!/bin/sh
# Builds gnuplot for GFOS (AmigaOS 3.2.3 with GFOpenUp) with a stove: 68040,
# soft float with the FPU taken when there is one (fpuauto), with the gfos
# terminal (amiga/gfos.trm: a window on the default public screen) as its
# default.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
# gnuplot's release tarball and fpuauto are fetched as gnuplot.lock gives;
# nothing third-party is kept here. A PC build of the same sources comes
# first (tests/host-gnuplot.sh, needs a PC compiler and python3): it makes
# the help file, gnuplot.gih, and checks the gfos terminal with gnuplot's
# own plots and demos.
# OUT gets Parts/gnuplot: a GFOpenUp part (its Part file, C:gnuplot, the
# help, manual page and copyright in SYS:Docs/gnuplot, gnuplotrc, colour
# sets and PostScript prologues in SYS:Share/gnuplot), installed with
#   GFOpenUpTool INSTALL Parts/gnuplot
# and demo/: gnuplot's demos, for a lab.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/gnuplot.lock"
. "$HERE/scripts/gp-common.sh"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK="${WORK:-$HOME/.cache/gfamigagnuplot/build}"; OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-8}
TC="$STOVE/prefix"; PATH="$TC/bin:$PATH"; export PATH
mkdir -p "$WORK/deps" "$OUT"; OUT=$(cd "$OUT" && pwd)
t0=$(date +%s)

# --- the PC build and tests first: the help file comes from it -----------------
WORK="$WORK/host" sh "$HERE/tests/host-gnuplot.sh" > "$WORK/host-tests.log" 2>&1 || { tail -40 "$WORK/host-tests.log"; exit 1; }
grep -E "demos through|checks passed" "$WORK/host-tests.log"
GIH="$WORK/host/build/docs/gnuplot.gih"

# --- fpuauto: libfpuauto.a and its link specs ---------------------------------
FPD="$WORK/deps/fpuauto"
if [ "$(git -C "$FPD" rev-parse HEAD 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FPD"; mkdir -p "$FPD"; git -C "$FPD" init -q
    git -C "$FPD" config core.sparseCheckout true
    echo "/common/fpuauto/" > "$FPD/.git/info/sparse-checkout"
    git -C "$FPD" fetch -q --depth 1 --filter=blob:none "$FPA_REPO" "$FPA_COMMIT"
    git -C "$FPD" checkout -q FETCH_HEAD
fi
FA="$FPD/common/fpuauto"; FAL="$WORK/fpuauto"
if [ ! -f "$FAL/libfpuauto.a" ] || [ "$(cat "$FAL/.rev" 2>/dev/null)" != "$FPA_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    m68k-amigaos-gcc -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        m68k-amigaos-gcc -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    m68k-amigaos-ar rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPA_COMMIT" > "$FAL/.rev"
fi

# --- gnuplot: the tarball, patches/, gfos.trm --------------------------------------
SRC="$WORK/gnuplot"
gp_source "$WORK/dl" "$SRC"
LATE=""
if m68k-amigaos-gcc -fno-late-combine-instructions -S -o /dev/null -x c /dev/null 2>/dev/null; then LATE="-fno-late-combine-instructions"; fi
# no loops turned into memset/memcpy calls (GCC 16), address 0 is memory
CF="-noixemul -m68040 -O2 -fno-tree-loop-distribute-patterns -fno-delete-null-pointer-checks $LATE"
LF="-noixemul -m68040 -specs=$HERE/scripts/m68k-soft-m68040.specs -specs=$FA/fpuauto.specs -L$FAL"

# --- the gfos terminal's window side, the stack and $VER ---------------------------
L="$WORK/gfos"; rm -rf "$L"; mkdir -p "$L"
for f in gfos_core gfos_amiga gp_compat; do
    m68k-amigaos-gcc $CF -Wall -I"$HERE/amiga" -c "$HERE/amiga/$f.c" -o "$L/$f.o"
done
m68k-amigaos-ar rcs "$L/libgfos.a" "$L/gfos_core.o" "$L/gfos_amiga.o" "$L/gp_compat.o"
m68k-amigaos-gcc $CF -DGP_AMIGA_VERSION=\"$GP_VERSION\" -DGP_AMIGA_DATE=\"$(date -u +%d.%m.%Y)\" \
    -c "$HERE/amiga/gp_amiga.c" -o "$L/gp_amiga.o"

# --- configure for m68k-amigaos (once per source), then make ------------------------
B="$WORK/amiga"
if [ "$(cat "$B/.src" 2>/dev/null)" != "$(cat "$SRC/.patched")-$(sha256sum "$0" | cut -c1-16)" ]; then
    rm -rf "$B"; mkdir -p "$B"
    # shellcheck disable=SC2086
    (cd "$B" && "$SRC/configure" --host=m68k-amigaos --build="$(uname -m)-pc-linux-gnu" \
        $GP_CONFIGURE --with-gihdir=SYS:Docs/gnuplot \
        CC=m68k-amigaos-gcc CFLAGS="$CF" CPPFLAGS="-DGFOS_TERM -I$HERE/amiga -include $HERE/amiga/gp_compat.h" LDFLAGS="$LF" \
        LIBS="$L/gp_amiga.o $L/libgfos.a" > configure.log 2>&1) || { tail -40 "$B/configure.log"; exit 1; }
    # the AmigaDOS console edits lines and keeps their history: gnuplot's
    # own line editor (raw terminal mode) is not used
    sed -i 's|^#define READLINE 1|/* READLINE: the AmigaDOS console edits lines */|' "$B/config.h"
    echo "$(cat "$SRC/.patched")-$(sha256sum "$0" | cut -c1-16)" > "$B/.src"
fi
echo "configure: $(( $(date +%s) - t0 )) s"
(cd "$B/src" && nice -n 19 make -k -j"$JOBS" gnuplot \
    HELPFILE=SYS:Docs/gnuplot/gnuplot.gih GNUPLOT_SHARE_DIR=SYS:Share/gnuplot \
    GNUPLOT_PS_DIR=SYS:Share/gnuplot/PostScript GNUPLOT_JS_DIR=SYS:Share/gnuplot/js \
    GNUPLOT_LUA_DIR=SYS:Share/gnuplot/lua > make.log 2>&1) || { grep -E 'error' "$B/src/make.log" | head -40; tail -20 "$B/src/make.log"; exit 1; }
echo "compile: $(( $(date +%s) - t0 )) s"

# --- the GFOpenUp part: Parts/gnuplot ---------------------------------------------
P="$OUT/Parts/gnuplot"; D="$P/Docs/gnuplot"; S="$P/Share/gnuplot"
rm -rf "$P"; mkdir -p "$P/C" "$D" "$S/PostScript"
m68k-amigaos-strip -o "$P/C/gnuplot" "$B/src/gnuplot"
cp "$HERE/README.md" "$D/ReadMe"
cp "$SRC/Copyright" "$D/Copyright"
cp "$GIH" "$D/gnuplot.gih"
cp "$SRC/man/man1/gnuplot.1" "$D/gnuplot.1"
cp "$SRC/share/gnuplotrc" "$SRC"/share/colors_*.gp "$S/"
cp "$SRC"/term/PostScript/*.ps "$SRC"/term/PostScript/aglfn.txt "$S/PostScript/"
sh "$HERE/scripts/write-part.sh" "$P" "$GP_VERSION"
# gnuplot's demos, for a lab (not in the part)
rm -rf "$OUT/demo"; cp -R "$SRC/demo" "$OUT/demo"
ls -l "$P/C/gnuplot"
echo "part: $(grep -c '^copy' "$P/Part") files, $(du -sk "$P" | cut -f1) KB"
echo "fpuauto: $(m68k-amigaos-nm "$B/src/gnuplot" | grep -c ' T ___wrap_') wrapped routines"
echo "total: $(( $(date +%s) - t0 )) s"
