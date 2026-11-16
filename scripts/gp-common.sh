# What build-amiga.sh (the stove) and tests/host-gnuplot.sh (the PC) share:
# fetching gnuplot, applying patches/ and putting amiga/gfos.trm into its
# term/ drawer. Sourced; needs HERE (the repo) and gnuplot.lock's settings.
# MIT, Copyright (c) 2026 AmiCradle Team.

# configure's switches both builds share: no X11, Qt, wx, cairo, Lua,
# libcerf, GD or plugins; the dumb, svg, PostScript, pbm (and the rest of
# the built-in file) terminals stay
GP_CONFIGURE="--without-x --with-qt=no --disable-wxwidgets --without-cairo
 --without-lua --without-libcerf --without-gd --without-caca --without-latex
 --with-readline=builtin --disable-plugins --disable-history-file
 --with-bitmap-terminals --without-tektronix"

# the release tarball, checked, into $1/gnuplot-<version>.tar.gz
gp_fetch() {
    t="$1/gnuplot-$GP_VERSION.tar.gz"
    if [ "$(sha256sum "$t" 2>/dev/null | cut -c1-64)" != "$GP_SHA256" ]; then
        mkdir -p "$1"; rm -f "$t"
        curl -sSfL -o "$t.part" "$GP_URL" || { echo "can't fetch $GP_URL"; exit 1; }
        [ "$(sha256sum "$t.part" | cut -c1-64)" = "$GP_SHA256" ] || { echo "$t: SHA-256 mismatch"; exit 1; }
        mv "$t.part" "$t"
    fi
}

# gnuplot unpacked into $2 from the tarball in $1, patches/ applied and
# gfos.trm in term/ (redone when any of them changes)
gp_source() {
    PSUM=$(cat "$HERE"/patches/*.patch "$HERE/amiga/gfos.trm" | sha256sum | cut -c1-16)
    if [ "$(cat "$2/.patched" 2>/dev/null)" != "$GP_SHA256-$PSUM" ]; then
        gp_fetch "$1"
        rm -rf "$2"; mkdir -p "$2"
        tar xzf "$1/gnuplot-$GP_VERSION.tar.gz" -C "$2" --strip-components=1
        for p in "$HERE"/patches/*.patch; do
            patch -s -d "$2" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
        done
        cp "$HERE/amiga/gfos.trm" "$2/term/gfos.trm"
        echo "$GP_SHA256-$PSUM" > "$2/.patched"
    fi
}
