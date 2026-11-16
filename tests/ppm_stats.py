#!/usr/bin/env python3
"""Facts about a P6 PPM picture for tests/host-gnuplot.sh:
  ppm_stats.py FILE size          -> "W H"
  ppm_stats.py FILE count RRGGBB  -> pixels of that colour
  ppm_stats.py FILE colours       -> distinct colours
  ppm_stats.py FILE rows RRGGBB   -> "first last" row holding that colour
MIT, Copyright (c) 2026 AmiCradle Team."""
import sys


def read(name):
    data = open(name, 'rb').read()
    fields, pos = [], 0
    while len(fields) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        start = pos
        while not data[pos:pos + 1].isspace():
            pos += 1
        fields.append(data[start:pos])
    if fields[0] != b'P6':
        sys.exit('not a P6 PPM')
    w, h = int(fields[1]), int(fields[2])
    return w, h, data[pos + 1:pos + 1 + 3 * w * h]


def main():
    w, h, pix = read(sys.argv[1])
    what = sys.argv[2]
    if what == 'size':
        print(w, h)
        return
    if what == 'colours':
        print(len({pix[i:i + 3] for i in range(0, len(pix), 3)}))
        return
    want = bytes.fromhex(sys.argv[3])
    if what == 'count':
        print(sum(1 for i in range(0, len(pix), 3) if pix[i:i + 3] == want))
    elif what == 'rows':
        rows = [i // 3 // w for i in range(0, len(pix), 3) if pix[i:i + 3] == want]
        print(min(rows), max(rows) if rows else '')


main()
