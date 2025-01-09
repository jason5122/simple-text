# TODO

# 01-09-2025

## Apply Zed's patch for scroll locking

- https://github.com/zed-industries/zed/pull/13867/files

## Implement alternative to `std::filesystem`

- https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++-features.md#std_filesystem-banned
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_path.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_path_watcher_win.cc
- https://learn.microsoft.com/en-us/windows/win32/fileio/createiocompletionport
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/memory_mapped_file_win.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_path_watcher_mac.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_path_watcher_kqueue.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/apple/scoped_dispatch_object.h;drc=763100e0bf9a25ba6f203612af5a4331fbd2d048;l=37
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_util_apple.mm
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_util_posix.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_util_win.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file.h
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_win.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_posix.cc
- https://source.chromium.org/chromium/chromium/src/+/main:base/threading/scoped_blocking_call_internal.cc;drc=763100e0bf9a25ba6f203612af5a4331fbd2d048;l=327
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/platform_file.h
- https://source.chromium.org/chromium/chromium/src/+/main:base/files/file_path.h

## Great GUI design example

- https://lisyarus.github.io/blog/posts/how-not-to-ui.html

## Look into better bin packing algorithm

- https://mozillagfx.wordpress.com/2021/02/04/improving-texture-atlas-allocation-in-webrender/
- https://github.com/nical/guillotiere/tree/main/cli
- https://tweedegolf.nl/en/blog/124/mix-in-rust-with-c

### LRU Cache

- https://github.com/lamerman/cpp-lru-cache/blob/master/include/lrucache.hpp
- https://source.chromium.org/chromium/chromium/src/+/main:content/browser/code_cache/simple_lru_cache.h
- https://source.chromium.org/chromium/chromium/src/+/main:content/browser/code_cache/simple_lru_cache.cc
- https://source.chromium.org/chromium/chromium/src/+/main:content/browser/code_cache/simple_lru_cache_unittest.cc
- https://source.chromium.org/chromium/chromium/src/+/main:third_party/leveldatabase/src/include/leveldb/cache.h
- https://source.chromium.org/chromium/chromium/src/+/main:third_party/leveldatabase/src/util/cache.cc

## Miscellaneous code snippets

- https://source.chromium.org/chromium/chromium/src/+/main:ui/gfx/mac/coordinate_conversion.mm
- https://source.chromium.org/chromium/chromium/src/+/main:base/time/time_apple.mm
- https://source.chromium.org/chromium/chromium/src/+/main:base/time/time.h
- https://github.com/ebassi/glarea-example/blob/master/glarea-app-window.c#L398

## Try out DXGI bitmap render target instead of WIC

- https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1factory-createdxgisurfacerendertarget(idxgisurface_constd2d1_render_target_properties__id2d1rendertarget)
- https://github.com/cmuratori/refterm/blob/29b21f757cf5a025eeb4069653b758a84a1f8689/refterm_example_dwrite.cpp#L31
- https://github.com/cmuratori/refterm/blob/29b21f757cf5a025eeb4069653b758a84a1f8689/refterm_example_dwrite.cpp#L171
- https://stackoverflow.com/questions/25780824/difference-between-id2d1bitmap-and-iwicbitmap
- https://stackoverflow.com/questions/2755180/windows-programming-id2d1bitmap-interface-getting-the-bitmap-data?rq=3
- https://learn.microsoft.com/en-us/windows/win32/Direct2D/supported-pixel-formats-and-alpha-modes#supported-formats-for-wic-bitmap-render-target
- https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nn-dxgi-idxgisurface
- https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dx-graphics-dxgi
- https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_mapped_rect
- https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_cpu_access_flag
- https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture2d
- https://stackoverflow.com/questions/75171099/how-to-get-pixel-data-out-of-an-idxgisurface-created-with-gpu-access-only

## Implement kinetic scrolling deceleration on GTK

- https://gitlab.gnome.org/GNOME/gtk/-/blob/main/gtk/gtkscrolledwindow.c?ref_type=heads#L173
- https://gitlab.gnome.org/GNOME/gtk/-/blob/filter-constructors/gtk/gtkkineticscrolling.c

## Learn from Chromium/Web animations

- https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/core/animation/README.md#The-interpolation-stack
- https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/core/animation/README.md#Timeline-Time
- https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/core/animation/README.md#Architecture
- https://www.w3.org/TR/web-animations-1/#timing-model
- https://lisyarus.github.io/blog/posts/exponential-smoothing.html

## Learn from Flutter's IME implementation on macOS

- https://github.com/flutter/flutter/issues/30725#issuecomment-948063626
- https://github.com/flutter/flutter/issues/78061
- https://github.com/flutter/engine/pull/32051
- https://github.com/flutter/flutter/issues/134699

# 12-31-2024

## Implement a proper logging system

- https://github.com/gabime/spdlog

## Implement pushing user-defined events

Note that we might have to implement our own event loop separate from the UI one.

- https://developer.apple.com/documentation/appkit/nsevent/eventtype/applicationdefined?language=objc
- https://source.chromium.org/chromium/chromium/src/+/main:third_party/wayland-protocols/gtk/gdk/macos/gdkmacoseventsource.c;l=324?q=otherEventWithType&ss=chromium
- https://developer.apple.com/documentation/appkit/nsevent/otherevent(with:location:modifierflags:timestamp:windownumber:context:subtype:data1:data2:)?language=objc
- https://stackoverflow.com/questions/48020222/how-to-make-nsapp-run-not-block/48023455#48023455
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagea
- https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#application-defined-messages
- https://forum.qt.io/topic/156527/update-ui-from-a-c-library-callback/5?lang=en-US
- https://docs.gtk.org/glib/main-loop.html

## Learn from Chromium's GUI toolkit

- https://chromium.googlesource.com/chromium/src/+/main/docs/ui/create/examples/login_dialog.md
- https://chromium.googlesource.com/chromium/src/+/main/docs/ui/animation_builder/animation_builder.md
- https://chromium.googlesource.com/chromium/src/+/main/docs/ui/input_event/index.md#Focus-Management

## Ensure OpenGL function loading is context-dependent

Apparently, functions are context-dependent on Windows.

- Take inspiration from datenwolf's `struct` approach [here](https://www.reddit.com/r/opengl/comments/17mq767/comment/k7mox6f/)
