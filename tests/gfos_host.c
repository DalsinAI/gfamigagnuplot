/* The PC side of gfos_backend.h, for the tests: the window is a picture in
 * memory, written as a PPM file to $GFOS_PPM at each finished plot, and
 * every call that isn't a line is logged to $GFOS_LOG ("open 640 480
 * gnuplot", "text 52 460 -10", "wait", ...). Text is not drawn, only logged.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfos_backend.h"

static unsigned char *pix;
static int W, H, open_, cx, cy;
static unsigned long colour;
static FILE *logf;

static void lg(const char *fmt, int a, int b, const char *s)
{
	const char *name = getenv("GFOS_LOG");
	if (!logf && name) logf = fopen(name, "a");
	if (!logf) return;
	fprintf(logf, fmt, a, b, s ? s : "");
	fputc('\n', logf);
	fflush(logf);
}

void gb_font_size(int *cw, int *ch, int *base) { *cw = 8; *ch = 8; *base = 6; }

int gb_open(int w, int h, const char *title)
{
	if (open_ && w == W && h == H) return 1;
	free(pix);
	pix = calloc((size_t)w * h, 3);
	if (!pix) return 0;
	W = w; H = h; open_ = 1;
	lg("open %d %d %s", w, h, title);
	return 1;
}

int gb_is_open(void) { return open_; }
void gb_close(void) { if (open_) lg("close", 0, 0, NULL); open_ = 0; }

static void plot(int x, int y)
{
	unsigned char *p;
	if (x < 0 || y < 0 || x >= W || y >= H) return;	/* the window clips */
	p = pix + 3 * ((size_t)y * W + x);
	p[0] = colour >> 16; p[1] = colour >> 8; p[2] = colour;
}

void gb_clear(unsigned long rgb)
{
	int i;
	for (i = 0; i < W * H; i++) { pix[3 * i] = rgb >> 16; pix[3 * i + 1] = rgb >> 8; pix[3 * i + 2] = rgb; }
}

void gb_colour(unsigned long rgb) { colour = rgb & 0xffffff; }
void gb_move(int x, int y) { cx = x; cy = y; }

void gb_draw(int x, int y)
{
	int dx = abs(x - cx), dy = -abs(y - cy), sx = cx < x ? 1 : -1, sy = cy < y ? 1 : -1, e = dx + dy;
	int px = cx, py = cy;
	for (;;) {
		plot(px, py);
		if (px == x && py == y) break;
		{ int e2 = 2 * e;
		  if (e2 >= dy) { e += dy; px += sx; }
		  if (e2 <= dx) { e += dx; py += sy; } }
	}
	cx = x; cy = y;
}

void gb_text(int x, int y, const char *s) { lg("text %d %d %s", x, y, s); }
int gb_text_width(const char *s) { return 8 * (int)strlen(s); }

/* even-odd scanline fill through pixel centres, as AreaFill does */
void gb_polygon(int n, const int *xy)
{
	int y, i, j, ymin = xy[1], ymax = xy[1], *xs = malloc(sizeof(int) * (n + 1));
	for (i = 1; i < n; i++) { if (xy[2 * i + 1] < ymin) ymin = xy[2 * i + 1]; if (xy[2 * i + 1] > ymax) ymax = xy[2 * i + 1]; }
	for (y = ymin; y <= ymax; y++) {
		int k = 0;
		for (i = 0, j = n - 1; i < n; j = i++) {
			int y0 = xy[2 * j + 1], y1 = xy[2 * i + 1], x0 = xy[2 * j], x1 = xy[2 * i];
			if ((y0 <= y && y < y1) || (y1 <= y && y < y0))
				xs[k++] = x0 + (int)((long)(y - y0) * (x1 - x0) / (y1 - y0));
		}
		for (i = 1; i < k; i++) for (j = i; j > 0 && xs[j - 1] > xs[j]; j--) { int t = xs[j]; xs[j] = xs[j - 1]; xs[j - 1] = t; }
		for (i = 0; i + 1 < k; i += 2) { int x; for (x = xs[i]; x <= xs[i + 1]; x++) plot(x, y); }
		if (k == 0 && ymin == ymax) { for (i = 0; i < n; i++) plot(xy[2 * i], y); }
	}
	/* the outline too, as AreaEnd draws it */
	for (i = 0; i < n; i++) { cx = xy[2 * i]; cy = xy[2 * i + 1]; gb_draw(xy[2 * ((i + 1) % n)], xy[2 * ((i + 1) % n) + 1]); }
	free(xs);
	lg("polygon %d %d", n, 0, NULL);
}

void gb_flush(void)
{
	const char *name = getenv("GFOS_PPM");
	FILE *fp;
	lg("flush", 0, 0, NULL);
	if (!name || !(fp = fopen(name, "wb"))) return;
	fprintf(fp, "P6\n%d %d\n255\n", W, H);
	fwrite(pix, 3, (size_t)W * H, fp);
	fclose(fp);
}

int gb_poll_close(void) { return 0; }
void gb_wait_close(void) { lg("wait", 0, 0, NULL); }
