/* gnuplot on AmigaOS: stack and $VER. Linked as an object (configure's
 * LIBS), so libnix's own __stack is not taken.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
/* gnuplot's parser and evaluator recurse; the Shell's 4 KB is far too little */
unsigned long __stack = 512 * 1024;
static const char version[] __attribute__((used)) =
	"$VER: gnuplot " GP_AMIGA_VERSION " (" GP_AMIGA_DATE ") GFOS";
