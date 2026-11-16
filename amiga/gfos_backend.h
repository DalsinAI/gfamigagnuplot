/* What gfos.trm (the gnuplot terminal for GFOS windows) draws with. The
 * Amiga side is gfos_amiga.c (an Intuition window, graphics.library); the
 * PC side is tests/gfos_host.c (a picture in memory written as a PPM file),
 * so the same terminal code is tested on the PC with gnuplot's own plots.
 * Coordinates are pixels in the window's inner area, (0,0) top left.
 * Colours are 0xRRGGBB. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GFOS_BACKEND_H
#define GFOS_BACKEND_H

/* the font every window of this size will get: character cell and baseline */
void gb_font_size(int *cell_w, int *cell_h, int *baseline);
/* opens (or keeps open) a w x h window; 1 when it is open */
int gb_open(int w, int h, const char *title);
int gb_is_open(void);
void gb_close(void);
void gb_clear(unsigned long rgb);
void gb_colour(unsigned long rgb);
void gb_move(int x, int y);
void gb_draw(int x, int y);
/* text with its cell's top left at x,y, in the current colour */
void gb_text(int x, int y, const char *s);
int gb_text_width(const char *s);
/* a filled polygon, xy holding n x,y pairs */
void gb_polygon(int n, const int *xy);
/* the plot is finished: show it */
void gb_flush(void);
/* 1 when the window's close gadget was clicked (it is then closed) */
int gb_poll_close(void);
/* waits for the close gadget, Esc, q or Ctrl-C (gnuplot -persist) */
void gb_wait_close(void);
#endif
