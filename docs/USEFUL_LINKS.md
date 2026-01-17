# Useful Links

## D3D11

- [Minimal D3D11 setup](https://gist.github.com/d7samurai/261c69490cce0620d0bfc93003cd1052)

## Chromium

- [Skia tab drawing code](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/views/tabs/tab_style_views.cc;l=370;drc=a438ffb4145448ea189d11daa7c9f8bdc2e77238)
- [GTK `dlopen()` example](https://source.chromium.org/chromium/chromium/src/+/main:ui/gtk/gtk_compat.cc;l=52;drc=a438ffb4145448ea189d11daa7c9f8bdc2e77238)
- [Chromium code coverage `BUILD.gn`](https://source.chromium.org/chromium/chromium/src/+/main:build/config/coverage/BUILD.gn;l=31;drc=59c007a7ad2f034baeffab754f3f5bf109b69cda)

- [UTF-8 encoder/decoder](https://mothereff.in/utf-8)
- [UTF summary](https://howardhinnant.github.io/utf_summary.html)
 
## Unicode test data

- [ICU UDHR `.txt` files](https://github.com/unicode-org/icu/tree/main/icu4j/perf-tests/data/udhr)
- [COSMIC Text `.txt` files](https://github.com/pop-os/cosmic-text/tree/main/sample)

## Text rendering

1. [Playing with UTF-8 in C++](https://mobiarch.wordpress.com/2022/12/03/playing-with-utf-8-in-c/)
2. [C++20 with u8, char8_t and std::string](https://stackoverflow.com/questions/56833000/c20-with-u8-char8-t-and-stdstring)
3. [Character Encoding in C++](https://www.reddit.com/r/cpp_questions/comments/q48m1q/character_encoding_in_c/)
4. [UTF-8 everywhere: How to do text on Windows](https://utf8everywhere.org/#windows)

### Unordered

- [Chromium `//base/threading/`](https://chromium.googlesource.com/chromium/src/base/+/refs/heads/main/threading)
- [Text Rendering Hates You](https://faultlore.com/blah/text-hates-you/#selection-isnt-a-box-and-text-goes-in-all-the-directions)
- [Laying Out Text With Core Text](https://robnapier.net/laying-out-text-with-coretext)
- [Modern text rendering with Linux: Overview](https://mrandri19.github.io/2019/07/24/modern-text-rendering-linux-overview.html)
- [Text layout is a loose hierarchy of segmentation](https://raphlinus.github.io/text/2020/10/26/text-layout.html)
- [Grapheme Clusters and Terminal Emulators](https://mitchellh.com/writing/grapheme-clusters-in-terminals)
- [Xi Editor `backspace.rs`](https://github.com/xi-editor/xi-editor/blob/master/rust/core-lib/src/backspace.rs)
- [Meaning of top, ascent, baseline, descent, bottom, and leading in Android's FontMetrics](https://stackoverflow.com/a/27631737/14698275)
- [Modern text rendering with Linux: Part 1](https://mrandri19.github.io/2019/07/18/modern-text-rendering-linux-ep1.html)
- [Harfbuzz: Core Text integration](https://harfbuzz.github.io/integration-coretext.html)
- [Android's Font Renderer](https://medium.com/@romainguy/androids-font-renderer-c368bbde87d9)
- [Improving the Font Pipeline](https://blog.hypersect.com/improving-the-font-pipeline/)
- [Adventures in Text Rendering: Kerning and Glyph Atlases](https://www.warp.dev/blog/adventures-text-rendering-kerning-glyph-atlases)
- [Complex text layout](https://en.wikipedia.org/wiki/Complex_text_layout)
- [FreeType glyph metrics SVG](https://freetype.org/freetype2/docs/tutorial/glyph-metrics-3.svg)

## Linux

- [Wayland Explorer](https://wayland.app/protocols/)

## C++

- [Comparisons in C++20](https://brevzin.github.io/c++/2019/07/28/comparisons-cpp20/)
- [Writing a custom iterator in modern C++](https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp)
- [Effortless Performance Improvements in C++: std::string_view](https://julien.jorge.st/posts/en/effortless-performance-improvements-in-cpp-std-string_view/)
  - [Reddit discussion](https://www.reddit.com/r/cpp/comments/11rs81y/effortless_performance_improvements_in_c/)

## Objective-C

- Delegates:
  - [Why use weak pointer for delegation?](https://stackoverflow.com/questions/8449040/why-use-weak-pointer-for-delegation)
  - [How to pass data from one MVC to another using Objective-C protocols](https://agilewarrior.wordpress.com/2012/08/31/how-to-pass-data-from-one-mvc-to-another-using-objective-c-protocols/)
- [NSString and Unicode](https://www.objc.io/issues/9-strings/unicode/)

## macOS

- [Apple Sample Code](https://developer.apple.com/library/archive/navigation/#section=Resource%20Types&topic=Sample%20Code)
- [Text System Defaults and Key Bindings](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/TextDefaultsBindings/TextDefaultsBindings.html#//apple_ref/doc/uid/20000468-CJBDEADF)
- [NSTrackingArea](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/EventOverview/TrackingAreaObjects/TrackingAreaObjects.html#//apple_ref/doc/uid/10000060i-CH8-SW1)
- [NSTextInputClient](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/TextEditing/Tasks/TextViewTask.html)
  - [How to check the status of input method in NSTextInputClient?](https://stackoverflow.com/questions/64932281/how-to-check-the-status-of-input-method-in-nstextinputclient)

## OpenGL

- [Async glTexSubImage2D and OGL thread blocking](https://stackoverflow.com/questions/6891274/async-gltexsubimage2d-and-ogl-thread-blocking)
