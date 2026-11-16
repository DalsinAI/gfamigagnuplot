# GFAmigaGnuplot

gnuplot 6.0.5, the plotting program, for GFOS (AmigaOS 3.2.3 with
GFOpenUp) on a 68040 or 68060, installed as a GFOpenUp part: `C:gnuplot`.
It draws in a window on Workbench (or any default public screen) through
its own terminal, `gfos`.

    1.Work:> gnuplot
    gnuplot> plot sin(x), cos(x)
    gnuplot> set pm3d; splot x*y with pm3d
    gnuplot> set terminal svg; set output "RAM:plot.svg"; replot

Status: built for the PC from the same tarball, patches and terminal code,
with the window replaced by a picture in memory, and passing the checks
below and ten of gnuplot's demos; the Amiga build is made by the stove,
not yet run in a lab.

## What you need

- GFOS: AmigaOS 3.2 with GFOpenUp, a 68040 or 68060. Soft float: the FPU
  is used when there is one (fpuauto), so an LC040 runs it too.
- Nothing else.

Install with `GFOpenUpTool INSTALL Parts/gnuplot`. `help` inside gnuplot
reads `SYS:Docs/gnuplot/gnuplot.gih`; the manual page is
`SYS:Docs/gnuplot/gnuplot.1`.

## The gfos terminal

    set terminal gfos {size <width>,<height>} {title "<text>"}

- The default terminal. `size` is the window's inside in pixels (640,480),
  `title` its title.
- Colours: the screen's pens, taken with `ObtainBestPen()` as colours come
  (rounded to 4 bits a channel, at most 256 held), so pm3d surfaces work
  on an 8-bit Workbench and look their best on RTG.
- Filled areas (boxes, filledcurves, pm3d) are solid, faded towards white
  by their density. Text is the screen's font, across only (rotated labels
  are set across).
- `gnuplot -persist script.plt` keeps the window open after gnuplot ends,
  until its close gadget, Esc, q or Ctrl-C. Mathomatic's `plot` command
  (DalsinAI/gfamigamathomatic) runs gnuplot this way.
- A window closed with its gadget during a session reopens at the next
  plot. No mousing (zoom, rotate with the mouse) yet.

The file terminals are all there: `svg`, `postscript`/`eps`, `pbm`, `dumb`,
`canvas` (HTML), `fig`, `cgm`, `emf`, `dxf`, `hpgl`, `pict2e`, `epslatex`
and the rest of the built-in set.

## On AmigaOS

- Line editing and history are the AmigaDOS console's (gnuplot's own line
  editor needs a raw Unix terminal, so it is not used).
- `gnuplotrc` and the colour sets are in `SYS:Share/gnuplot`, the
  PostScript prologues in `SYS:Share/gnuplot/PostScript`.
- 512 KB of stack is set in the program, so the Shell's 4 KB is enough.
- Not built in: X11, Qt, wxWidgets, cairo (`pngcairo`, `pdfcairo`), Lua
  (`tikz`), libgd (`png`), libcerf, plugins.

## Building

    STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out sh build-amiga.sh

or in Cradle, stove_build with the os32-gcc16-open6 stove, target `.`.
The gnuplot 6.0.5 tarball is fetched and checked against the SHA-256 in
`gnuplot.lock`, fpuauto at its pinned commit. The PC build runs first (it
makes the help file), then gnuplot's own `configure --host=m68k-amigaos`
and `make`. `OUT` gets `Parts/gnuplot` (the part) and `demo/` (gnuplot's
demos, for a lab).

## Tests

`make -C tests test` on a PC:

- `tests/test-part.sh`: the Part file;
- `tests/core_test.c`: `amiga/gfos_core.c` (line colours, fades, flipping,
  text placement, line clipping to the window, the pen table);
- `tests/host-gnuplot.sh`: gnuplot built from the same sources with
  `GFOS_TERM` and `tests/gfos_host.c` in place of the window (a PPM
  picture and a log of the calls). It checks that gfos is listed and the
  default; a `sin(x)` plot's size, line colour, background, border and
  labels; which way up y goes; solid and faded boxes; a pm3d surface's
  colours; `size` and `title`; `-persist` waiting for the window (and not
  without it); a bad size refused; the script Mathomatic writes; `help
  gfos`; and ten of gnuplot's demos run through the terminal.

## What was changed

- `patches/0001`: `src/term.h` takes `gfos.trm` when `GFOS_TERM` is
  defined; `src/term.c` makes it the default terminal.
- `amiga/gfos.trm`: the terminal (options, lines, text, fills, colours,
  help text); copied into gnuplot's `term/` drawer at build time.
- `amiga/gfos_core.c`: its arithmetic, tested on the PC.
- `amiga/gfos_amiga.c`: the window: Intuition, graphics.library pens,
  `AreaDraw` fills with a TmpRas, `TextLength`/`Text`, IDCMP for closing.
- `amiga/gp_amiga.c`: stack and `$VER`.

gnuplot is under its own licence (`SYS:Docs/gnuplot/Copyright`); this
port's own files are MIT.

Designed, tested and delivered by The AmiCradle Team.
