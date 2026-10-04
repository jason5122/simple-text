from __future__ import annotations

from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.misc.timeTools import timestampSinceEpoch
from fontTools.pens.ttGlyphPen import TTGlyphPen

HERE = Path(__file__).resolve().parent

# Ahem's box: 1000 units per em, ascent 800, descent 200.
EM = 1000
ASCENT = 800
DESCENT = -200


def box(x0: int, y0: int, x1: int, y1: int):
    pen = TTGlyphPen(None)
    pen.moveTo((x0, y0))
    pen.lineTo((x0, y1))
    pen.lineTo((x1, y1))
    pen.lineTo((x1, y0))
    pen.closePath()
    return pen.glyph()


def empty():
    return TTGlyphPen(None).glyph()


def em_box(width: int = EM):
    return box(0, DESCENT, width, ASCENT)


def build(
    name: str,
    *,
    upem: int,
    ascent: int,
    descent: int,
    line_gap: int = 0,
    glyphs: dict,
    advances: dict,
    cmap: dict,
    features: str | None = None,
) -> None:
    """`glyphs` is an ordered name -> outline mapping whose first entry must be .notdef."""
    fb = FontBuilder(upem)
    # A fixed timestamp keeps the file bytes reproducible from one run to the next.
    fb.updateHead(created=timestampSinceEpoch(0), modified=timestampSinceEpoch(0))
    order = list(glyphs)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)
    fb.setupGlyf(glyphs)
    glyf = fb.font["glyf"]
    fb.setupHorizontalMetrics({g: (advances[g], getattr(glyf[g], "xMin", 0)) for g in order})
    fb.setupHorizontalHeader(ascent=ascent, descent=descent, lineGap=line_gap)
    family = "SimpleText " + name.replace("_", " ").title()
    ps_name = "SimpleText-" + name.replace("_", "-")
    fb.setupNameTable(
        dict(
            familyName=family,
            styleName="Regular",
            uniqueFontIdentifier=ps_name,
            fullName=family,
            psName=ps_name,
            version="Version 1.0",
        )
    )
    # usWinAscent/Descent are unsigned; clamp rather than let fontTools raise on a negative.
    fb.setupOS2(
        sTypoAscender=ascent,
        sTypoDescender=descent,
        sTypoLineGap=line_gap,
        usWinAscent=min(max(ascent, 0), 0xFFFF),
        usWinDescent=min(max(-descent, 0), 0xFFFF),
        fsSelection=0x80 | 0x40,  # USE_TYPO_METRICS | REGULAR
        version=4,  # the first OS/2 version that defines USE_TYPO_METRICS
    )
    if features:
        fb.addOpenTypeFeatures(features)
    fb.setupPost()
    fb.save(HERE / f"{name}.ttf")


def main() -> None:
    # A glyph whose scratch buffer would be gigabytes. 16 units per em is the smallest TrueType
    # allows, and 32000 units of that is 2000 em: at 20px the square is 40000 device pixels on a
    # side, or 6.4 GB of BGRA.
    build(
        "giant",
        upem=16,
        ascent=13,
        descent=-3,
        glyphs={".notdef": empty(), "A": box(0, 0, 32000, 32000)},
        advances={".notdef": 16, "A": 32000},
        cmap={ord("A"): "A"},
    )

    # A 100 em square: 2000 device pixels on a side at 20px, which is large but rasterizable. Not
    # derived from the 16-unit font above because Core Text draws glyphs this large from a font
    # with so few units per em at the wrong scale, 0.8 of the advance it reports for them.
    build(
        "large",
        upem=100,
        ascent=80,
        descent=-20,
        glyphs={".notdef": empty(), "A": box(0, 0, 10000, 10000)},
        advances={".notdef": 100, "A": 10000},
        cmap={ord("A"): "A"},
    )

    # A small glyph placed 2000 em to the left of and below the origin. Its scratch buffer is
    # modest, but its bearings are tens of thousands of pixels and cannot fit the cache's 16-bit
    # phase record.
    build(
        "far",
        upem=16,
        ascent=13,
        descent=-3,
        glyphs={".notdef": empty(), "A": box(-32000, -32000, -31000, -31000)},
        advances={".notdef": 16, "A": 16},
        cmap={ord("A"): "A"},
    )

    # Ascent, descent and line gap all at the int16 limit: 32.767 em each.
    build(
        "huge_metrics",
        upem=EM,
        ascent=32767,
        descent=-32767,
        line_gap=32767,
        glyphs={".notdef": empty(), "space": empty(), "A": em_box()},
        advances={".notdef": EM, "space": EM, "A": EM},
        cmap={ord(" "): "space", ord("A"): "A"},
    )

    # Ascent below the baseline and descent above it.
    build(
        "negative_metrics",
        upem=EM,
        ascent=-100,
        descent=100,
        glyphs={".notdef": empty(), "space": empty(), "A": em_box()},
        advances={".notdef": EM, "space": EM, "A": EM},
        cmap={ord(" "): "space", ord("A"): "A"},
    )

    # Zero units per em: every design-unit-to-pixel scale is a division by zero.
    build(
        "zero_upem",
        upem=0,
        ascent=ASCENT,
        descent=DESCENT,
        glyphs={".notdef": empty(), "A": em_box()},
        advances={".notdef": EM, "A": EM},
        cmap={ord("A"): "A"},
    )

    # A single empty .notdef and no character map, so every character falls back.
    build(
        "notdef_only",
        upem=EM,
        ascent=ASCENT,
        descent=DESCENT,
        glyphs={".notdef": empty()},
        advances={".notdef": EM},
        cmap={},
    )

    # Ink without advance. M and i are mapped so fx_font::widths() stays inside the file.
    letters = {c: "box" for c in "ABMi"}
    build(
        "zero_advance",
        upem=EM,
        ascent=ASCENT,
        descent=DESCENT,
        glyphs={".notdef": empty(), "space": empty(), "box": em_box()},
        advances={".notdef": 0, "space": 0, "box": 0},
        cmap={ord(" "): "space", **{ord(c): g for c, g in letters.items()}},
    )

    # One glyph per OpenType feature fx can switch, each substitution changing the advance so a
    # test can tell from fx_layout::advance alone whether the feature applied:
    #   liga  f i -> f_i   (1500)      clig  s t -> s_t   (1500)
    #   dlig  c t -> c_t   (1500)      calt  a before b -> a.calt (750)
    #   ss01  a -> a.ss01  (500)       ss10  b -> b.ss10  (250)
    #   kern  A V          (-500)
    plain = "fictsabAV"
    build(
        "features",
        upem=EM,
        ascent=ASCENT,
        descent=DESCENT,
        glyphs={
            ".notdef": empty(),
            "space": empty(),
            **{c: em_box() for c in plain},
            "f_i": em_box(1500),
            "s_t": em_box(1500),
            "c_t": em_box(1500),
            "a.calt": em_box(750),
            "a.ss01": em_box(500),
            "b.ss10": em_box(250),
        },
        advances={
            ".notdef": EM,
            "space": EM,
            **{c: EM for c in plain},
            "f_i": 1500,
            "s_t": 1500,
            "c_t": 1500,
            "a.calt": 750,
            "a.ss01": 500,
            "b.ss10": 250,
        },
        cmap={ord(" "): "space", **{ord(c): c for c in plain}},
        features="""
            feature liga { sub f i by f_i; } liga;
            feature clig { sub s t by s_t; } clig;
            feature dlig { sub c t by c_t; } dlig;
            feature calt { sub a' b by a.calt; } calt;
            feature ss01 { sub a by a.ss01; } ss01;
            feature ss10 { sub b by b.ss10; } ss10;
            feature kern { pos A V -500; } kern;
        """,
    )

    # Files that are not fonts at all.
    ahem = (HERE / "Ahem.ttf").read_bytes()
    (HERE / "truncated.ttf").write_bytes(ahem[:512])
    (HERE / "text.ttf").write_bytes(b"This is not a font.\n")
    (HERE / "zero_bytes.ttf").write_bytes(b"")


if __name__ == "__main__":
    main()
