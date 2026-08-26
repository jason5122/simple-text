#pragma once

#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/layout.h"
#include <functional>
#include <string>
#include <vector>

// A crop rectangle in device pixels, top-left origin. A zero-size rect means the whole window.
struct Crop {
    int x = 0, y = 0, w = 0, h = 0;
};

// Lays out a page of text for a given font. The interactive window calls this at startup and again
// on every runtime font change, so each call fully re-shapes and re-rasterizes for the new
// face/size.
using SourceProvider = std::function<GlyphAtlasSource(const font::FontSpec&)>;

// Opens a window that draws each glyph as its own textured quad sampled from a glyph atlas, plus
// the whole atlas in the upper-right corner as a debug view. The user can change the font live:
//   - / +  shrink / grow the size      [ / ]  previous / next family (cycles `families`)
//   b      toggle bold                 i      toggle italic
// Each change re-runs `provider` and rebuilds the atlas. `initial` is the starting font (its
// family must be `families.front()`); `scale` is the device-pixel ratio the glyphs are positioned
// at, used as the layer's contents scale.
void run_text_window(const font::FontSpec& initial,
                     std::vector<std::string> families,
                     double scale,
                     SourceProvider provider);

// One screenshot to produce: a font + the lines of text to render, and where to save the PNG.
struct TestShot {
    font::FontSpec font;
    std::vector<std::string> lines;
    std::string out_path;
};

// Runs the screenshot test suite in a single persistent window: for each shot, swaps in the new
// font/text and captures the window (cropped to `crop`) once the frame settles -- no per-shot
// relaunch, no fixed delays. `scale` is the device-pixel ratio.
void run_test_window(std::vector<TestShot> shots, Crop crop, double scale);
