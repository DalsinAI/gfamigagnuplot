/* The AmigaOS side of gfos_backend.h: gnuplot's gfos terminal draws in an
 * Intuition window on the default public screen (normally Workbench) with
 * graphics.library. Colours become pens through ObtainBestPen(), kept in
 * gfos_core.c's table; filled polygons go through AreaMove/AreaDraw with a
 * TmpRas the size of the window. Smart refresh: Intuition keeps what is
 * covered, so nothing is redrawn by hand.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <string.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <graphics/view.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include "gfos_backend.h"
#include "gfos_core.h"

static struct Window *win;
static struct Screen *scr;		/* locked while the window is open */
static struct TmpRas tmpras;
static PLANEPTR tmpplane;
static int iw, ih, ox, oy;		/* inner size, inner offset */
static struct gfos_pens pens;
static char wtitle[80];

static long obtain(unsigned long rgb, void *ctx)
{
	struct ColorMap *cm = (struct ColorMap *)ctx;
	ULONG r = (rgb >> 16) & 0xff, g = (rgb >> 8) & 0xff, b = rgb & 0xff;
	LONG pen = ObtainBestPen(cm, r * 0x01010101UL, g * 0x01010101UL, b * 0x01010101UL,
	                         OBP_Precision, PRECISION_IMAGE, TAG_DONE);
	return pen < 0 ? -1 : pen;
}

static void release(long pen, void *ctx)
{
	ReleasePen((struct ColorMap *)ctx, (ULONG)pen);
}

void gb_font_size(int *cw, int *ch, int *base)
{
	struct Screen *s = LockPubScreen(NULL);
	*cw = 8; *ch = 8; *base = 6;
	if (s) {
		struct TextFont *tf = s->RastPort.Font;
		if (tf) { *cw = tf->tf_XSize; *ch = tf->tf_YSize; *base = tf->tf_Baseline; }
		UnlockPubScreen(NULL, s);
	}
}

int gb_is_open(void) { return win != NULL; }

void gb_close(void)
{
	if (!win) return;
	gfos_pens_release_all(&pens, release, scr->ViewPort.ColorMap);
	win->RPort->TmpRas = NULL;
	if (tmpplane) FreeRaster(tmpplane, iw, ih);
	tmpplane = NULL;
	CloseWindow(win);
	win = NULL;
	UnlockPubScreen(NULL, scr);
	scr = NULL;
}

int gb_open(int w, int h, const char *title)
{
	if (win && w == iw && h == ih) {
		if (strcmp(title, wtitle) != 0) {
			strncpy(wtitle, title, sizeof(wtitle) - 1);
			SetWindowTitles(win, (CONST_STRPTR)wtitle, (CONST_STRPTR)~0);
		}
		return 1;
	}
	gb_close();
	strncpy(wtitle, title, sizeof(wtitle) - 1);
	wtitle[sizeof(wtitle) - 1] = 0;
	if (!(scr = LockPubScreen(NULL))) return 0;
	win = OpenWindowTags(NULL,
		WA_PubScreen, (ULONG)scr,
		WA_InnerWidth, w, WA_InnerHeight, h,
		WA_Title, (ULONG)wtitle,
		WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE,
		WA_SmartRefresh, TRUE, WA_AutoAdjust, TRUE, WA_Activate, FALSE,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY,
		TAG_DONE);
	if (!win) { UnlockPubScreen(NULL, scr); scr = NULL; return 0; }
	iw = w; ih = h; ox = win->BorderLeft; oy = win->BorderTop;
	gfos_pens_init(&pens);
	/* AreaFill's work bitmap: one plane the size of the drawing area */
	if ((tmpplane = AllocRaster(iw, ih)) != NULL) {
		InitTmpRas(&tmpras, tmpplane, RASSIZE(iw, ih));
		win->RPort->TmpRas = &tmpras;
	}
	SetDrMd(win->RPort, JAM1);
	return 1;
}

void gb_colour(unsigned long rgb)
{
	long pen;
	if (!win) return;
	pen = gfos_pen_for(&pens, rgb, obtain, release, scr->ViewPort.ColorMap);
	SetAPen(win->RPort, pen >= 0 ? (ULONG)pen : 1);
}

void gb_clear(unsigned long rgb)
{
	if (!win) return;
	gb_colour(rgb);
	RectFill(win->RPort, ox, oy, ox + iw - 1, oy + ih - 1);
}

void gb_move(int x, int y) { if (win) Move(win->RPort, ox + x, oy + y); }
void gb_draw(int x, int y) { if (win) Draw(win->RPort, ox + x, oy + y); }

void gb_text(int x, int y, const char *s)
{
	struct RastPort *rp;
	if (!win) return;
	rp = win->RPort;
	Move(rp, ox + x, oy + y + rp->TxBaseline);
	Text(rp, (CONST_STRPTR)s, strlen(s));
}

int gb_text_width(const char *s)
{
	struct RastPort rp;
	if (win) return TextLength(win->RPort, (CONST_STRPTR)s, strlen(s));
	InitRastPort(&rp);
	return (int)strlen(s) * 8;
}

void gb_polygon(int n, const int *xy)
{
	struct AreaInfo ai;
	struct RastPort *rp;
	WORD *buf;
	int i;
	if (!win || n < 3) return;
	rp = win->RPort;
	if (!rp->TmpRas) {	/* no work bitmap: the outline only */
		Move(rp, ox + xy[0], oy + xy[1]);
		for (i = 1; i <= n; i++) Draw(rp, ox + xy[2 * (i % n)], oy + xy[2 * (i % n) + 1]);
		return;
	}
	/* AreaInfo wants 5 bytes a vertex, word aligned, one more for the close */
	if (!(buf = AllocVec((n + 2) * 5 + 2, MEMF_CLEAR))) return;
	InitArea(&ai, buf, n + 2);
	rp->AreaInfo = &ai;
	AreaMove(rp, ox + xy[0], oy + xy[1]);
	for (i = 1; i < n; i++) AreaDraw(rp, ox + xy[2 * i], oy + xy[2 * i + 1]);
	AreaEnd(rp);
	rp->AreaInfo = NULL;
	FreeVec(buf);
}

void gb_flush(void) { /* drawing goes straight to the window */ }

/* 1 for a message that closes: the close gadget, Esc or q */
static int closing(struct IntuiMessage *m)
{
	return m->Class == IDCMP_CLOSEWINDOW ||
	       (m->Class == IDCMP_VANILLAKEY && (m->Code == 27 || m->Code == 'q' || m->Code == 'Q'));
}

int gb_poll_close(void)
{
	struct IntuiMessage *m;
	int close = 0;
	if (!win) return 0;
	while ((m = (struct IntuiMessage *)GetMsg(win->UserPort)) != NULL) {
		if (closing(m)) close = 1;
		ReplyMsg((struct Message *)m);
	}
	if (close) gb_close();
	return close;
}

void gb_wait_close(void)
{
	while (win) {
		ULONG got = Wait(1UL << win->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);
		if (got & SIGBREAKF_CTRL_C) { gb_close(); break; }
		gb_poll_close();
	}
}
