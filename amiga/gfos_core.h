/* gfos.trm's arithmetic, kept out of the drawing so the PC tests reach it
 * (tests/core_test.c). MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFOS_CORE_H
#define GFOS_CORE_H

/* gnuplot's special line types (term_api.h) */
#define GFOS_LT_AXIS       (-1)
#define GFOS_LT_BLACK      (-2)
#define GFOS_LT_NODRAW     (-3)
#define GFOS_LT_BACKGROUND (-4)

/* the colour of line type lt: white background, black, grey axes, then the
 * classic eight (red, green, blue, magenta, cyan, sienna, orange, coral) */
unsigned long gfos_lt_rgb(int lt);
/* a solid fill of density 0..100: the colour faded towards the background */
unsigned long gfos_shade(unsigned long rgb, int density, unsigned long background);
/* gnuplot counts y up from the bottom; the window down from the top */
int gfos_flip_y(int y, int height);
/* where text of this width starts for justification 0 left, 1 centre, 2 right */
int gfos_justify_x(int x, int width, int justify);

/* Clips the line (x0,y0)-(x1,y1) to 0..w-1, 0..h-1 (graphics.library
 * takes 16-bit coordinates); 0 when none of it is inside. */
int gfos_clip_line(int *x0, int *y0, int *x1, int *y1, int w, int h);

/* Pens: the screen has few, a pm3d surface asks for thousands of colours.
 * Colours are rounded to 4 bits a channel and kept in a table of
 * GFOS_PENS; a new colour that lands on a used slot releases that pen. */
#define GFOS_PENS 256
struct gfos_pens {
	unsigned long rgb[GFOS_PENS];	/* rounded colour, or ~0 for free */
	long pen[GFOS_PENS];
	int obtained, released;		/* counts, for the tests */
};
typedef long (*gfos_obtain_fn)(unsigned long rgb, void *ctx);	/* -1 when none */
typedef void (*gfos_release_fn)(long pen, void *ctx);
unsigned long gfos_round_rgb(unsigned long rgb);
void gfos_pens_init(struct gfos_pens *p);
/* the pen for rgb, obtaining it when it isn't held; -1 when none */
long gfos_pen_for(struct gfos_pens *p, unsigned long rgb, gfos_obtain_fn ob, gfos_release_fn rel, void *ctx);
void gfos_pens_release_all(struct gfos_pens *p, gfos_release_fn rel, void *ctx);
#endif
