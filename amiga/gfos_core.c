/* gfos.trm's arithmetic (see gfos_core.h). MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gfos_core.h"

static const unsigned long classic[8] = {
	0xff0000, 0x00c000, 0x0080ff, 0xc000ff, 0x00eeee, 0xc04000, 0xeeee00, 0x2020c0
};

unsigned long gfos_lt_rgb(int lt)
{
	switch (lt) {
	case GFOS_LT_BACKGROUND:
	case GFOS_LT_NODRAW: return 0xffffff;
	case GFOS_LT_AXIS: return 0xa0a0a0;
	case GFOS_LT_BLACK: return 0x000000;
	}
	if (lt < 0) return 0x000000;
	return classic[lt % 8];
}

unsigned long gfos_shade(unsigned long rgb, int density, unsigned long bg)
{
	unsigned long out = 0;
	int shift;
	if (density >= 100) return rgb & 0xffffff;
	if (density <= 0) return bg & 0xffffff;
	for (shift = 0; shift <= 16; shift += 8) {
		int c = (int)((rgb >> shift) & 0xff), b = (int)((bg >> shift) & 0xff);
		int v = (c * density + b * (100 - density) + 50) / 100;
		out |= (unsigned long)(v & 0xff) << shift;
	}
	return out;
}

int gfos_flip_y(int y, int height)
{
	return height - 1 - y;
}

int gfos_justify_x(int x, int width, int justify)
{
	if (justify == 1) return x - width / 2;
	if (justify == 2) return x - width;
	return x;
}

static int outcode(int x, int y, int w, int h)
{
	return (x < 0) | (x >= w) << 1 | (y < 0) << 2 | (y >= h) << 3;
}

int gfos_clip_line(int *x0, int *y0, int *x1, int *y1, int w, int h)
{
	/* Cohen-Sutherland, in double so far-off points don't overflow */
	double ax = *x0, ay = *y0, bx = *x1, by = *y1;
	int a = outcode(*x0, *y0, w, h), b = outcode(*x1, *y1, w, h), round;
	for (round = 0; ; round++) {
		double x, y;
		int c;
		if (!(a | b)) break;
		if ((a & b) || round == 8) return 0;
		c = a ? a : b;
		if (c & 8)      { x = ax + (bx - ax) * (h - 1 - ay) / (by - ay); y = h - 1; }
		else if (c & 4) { x = ax + (bx - ax) * (0 - ay) / (by - ay); y = 0; }
		else if (c & 2) { y = ay + (by - ay) * (w - 1 - ax) / (bx - ax); x = w - 1; }
		else            { y = ay + (by - ay) * (0 - ax) / (bx - ax); x = 0; }
		if (c == a) { ax = x; ay = y; a = outcode((int)(x + 0.5 * (x >= 0 ? 1 : -1)), (int)(y + 0.5 * (y >= 0 ? 1 : -1)), w, h); }
		else        { bx = x; by = y; b = outcode((int)(x + 0.5 * (x >= 0 ? 1 : -1)), (int)(y + 0.5 * (y >= 0 ? 1 : -1)), w, h); }
	}
	*x0 = (int)(ax + 0.5); *y0 = (int)(ay + 0.5); *x1 = (int)(bx + 0.5); *y1 = (int)(by + 0.5);
	return 1;
}

unsigned long gfos_round_rgb(unsigned long rgb)
{
	/* 4 bits a channel, repeated into the low nibble (0xf0 -> 0xff) */
	unsigned long r = (rgb >> 20) & 0xf, g = (rgb >> 12) & 0xf, b = (rgb >> 4) & 0xf;
	return (r << 20 | r << 16) | (g << 12 | g << 8) | (b << 4 | b);
}

void gfos_pens_init(struct gfos_pens *p)
{
	int i;
	for (i = 0; i < GFOS_PENS; i++) { p->rgb[i] = ~0UL; p->pen[i] = -1; }
	p->obtained = p->released = 0;
}

static unsigned slot_of(unsigned long rgb)
{
	/* 12 bits of colour folded to 8 */
	unsigned long k = ((rgb >> 20) & 0xf) << 8 | ((rgb >> 12) & 0xf) << 4 | ((rgb >> 4) & 0xf);
	return (unsigned)((k ^ (k >> 8) * 97) & (GFOS_PENS - 1));
}

long gfos_pen_for(struct gfos_pens *p, unsigned long rgb, gfos_obtain_fn ob, gfos_release_fn rel, void *ctx)
{
	unsigned long want = gfos_round_rgb(rgb);
	unsigned s = slot_of(want);
	long pen;
	if (p->rgb[s] == want) return p->pen[s];
	if (p->rgb[s] != ~0UL) {
		if (p->pen[s] >= 0) { rel(p->pen[s], ctx); p->released++; }
		p->rgb[s] = ~0UL; p->pen[s] = -1;
	}
	pen = ob(want, ctx);
	if (pen < 0) return -1;
	p->obtained++;
	p->rgb[s] = want; p->pen[s] = pen;
	return pen;
}

void gfos_pens_release_all(struct gfos_pens *p, gfos_release_fn rel, void *ctx)
{
	int i;
	for (i = 0; i < GFOS_PENS; i++) {
		if (p->rgb[i] != ~0UL && p->pen[i] >= 0) { rel(p->pen[i], ctx); p->released++; }
		p->rgb[i] = ~0UL; p->pen[i] = -1;
	}
}
