/* PC test of amiga/gfos_core.c: line colours, fades, flipping, text
 * placement, line clipping and the pen table.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include "gfos_core.h"

static int fails, checks;
static void ok(int c, const char *what) { checks++; if (!c) { fails++; printf("FAIL %s\n", what); } }

static long next_pen;
static long ob(unsigned long rgb, void *ctx) { (void)rgb; (void)ctx; return next_pen < 0 ? -1 : next_pen++; }
static int held;
static void rel(long pen, void *ctx) { (void)pen; (void)ctx; held--; }
static long ob_count(unsigned long rgb, void *ctx) { held++; return ob(rgb, ctx); }

int main(void)
{
	struct gfos_pens p;
	int x0, y0, x1, y1, i;
	unsigned long r, g;
	long a, b;

	ok(gfos_lt_rgb(GFOS_LT_BACKGROUND) == 0xffffff, "background white");
	ok(gfos_lt_rgb(GFOS_LT_BLACK) == 0x000000, "black");
	ok(gfos_lt_rgb(GFOS_LT_AXIS) == 0xa0a0a0, "axis grey");
	ok(gfos_lt_rgb(0) == 0xff0000, "lt 0 red");
	ok(gfos_lt_rgb(8) == gfos_lt_rgb(0), "eight then again");
	ok(gfos_lt_rgb(-9) == 0x000000, "other negative black");

	ok(gfos_shade(0x0000ff, 100, 0xffffff) == 0x0000ff, "density 100 is the colour");
	ok(gfos_shade(0x0000ff, 0, 0xffffff) == 0xffffff, "density 0 is the background");
	ok(gfos_shade(0x0000ff, 50, 0xffffff) == 0x8080ff, "density 50 halfway");
	ok(gfos_shade(0xffffff, 50, 0x000000) == 0x808080, "fade towards black");

	ok(gfos_flip_y(0, 480) == 479 && gfos_flip_y(479, 480) == 0, "flip");
	ok(gfos_justify_x(100, 40, 0) == 100 && gfos_justify_x(100, 40, 1) == 80 && gfos_justify_x(100, 40, 2) == 60, "justify");

	x0 = 10; y0 = 10; x1 = 20; y1 = 20;
	ok(gfos_clip_line(&x0, &y0, &x1, &y1, 100, 100) && x0 == 10 && y1 == 20, "inside unchanged");
	x0 = -50; y0 = 50; x1 = 150; y1 = 50;
	ok(gfos_clip_line(&x0, &y0, &x1, &y1, 100, 100) && x0 == 0 && x1 == 99 && y0 == 50 && y1 == 50, "across clipped to both sides");
	x0 = -10; y0 = -10; x1 = -5; y1 = 200;
	ok(!gfos_clip_line(&x0, &y0, &x1, &y1, 100, 100), "all left: nothing");
	x0 = 50; y0 = 50; x1 = 2000000000; y1 = 50;
	ok(gfos_clip_line(&x0, &y0, &x1, &y1, 100, 100) && x1 == 99, "far off point");
	x0 = -100; y0 = -100; x1 = 200; y1 = 200;
	ok(gfos_clip_line(&x0, &y0, &x1, &y1, 100, 100) && x0 == 0 && y0 == 0 && x1 == 99 && y1 == 99, "diagonal through corners");
	for (i = 0; i < 2000; i++) {	/* every result lies in the window */
		x0 = (i * 7919) % 3000 - 1500; y0 = (i * 104729) % 3000 - 1500;
		x1 = (i * 1299709) % 3000 - 1500; y1 = (i * 15485863) % 3000 - 1500;
		if (gfos_clip_line(&x0, &y0, &x1, &y1, 320, 200) &&
		    (x0 < 0 || x0 > 319 || x1 < 0 || x1 > 319 || y0 < 0 || y0 > 199 || y1 < 0 || y1 > 199)) {
			ok(0, "clipped line outside the window"); break;
		}
	}

	ok(gfos_round_rgb(0x9400d3) == 0x9900dd, "rounded to 4 bits a channel");
	gfos_pens_init(&p);
	next_pen = 4; held = 0;
	a = gfos_pen_for(&p, 0xff0000, ob_count, rel, NULL);
	b = gfos_pen_for(&p, 0xf8000f, ob_count, rel, NULL);
	ok(a == 4 && b == 4 && p.obtained == 1, "near colours share a pen");
	ok(gfos_pen_for(&p, 0x00ff00, ob_count, rel, NULL) == 5, "a new colour, a new pen");
	for (r = 0; r < 16; r++) for (g = 0; g < 16; g++) gfos_pen_for(&p, r << 20 | g << 12 | 0x40, ob_count, rel, NULL);
	ok(held <= GFOS_PENS, "never more pens than the table holds");
	gfos_pens_release_all(&p, rel, NULL);
	ok(held == 0, "all pens released");
	next_pen = -1;
	ok(gfos_pen_for(&p, 0x123456, ob, rel, NULL) == -1, "no pen to be had");

	printf("gfos_core: %d of %d checks passed\n", checks - fails, checks);
	return fails != 0;
}
