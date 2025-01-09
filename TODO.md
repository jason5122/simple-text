# TODO

# 01-09-2025

### Try out DXGI bitmap render target instead of WIC

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

### Implement kinetic scrolling deceleration on GTK

- https://gitlab.gnome.org/GNOME/gtk/-/blob/main/gtk/gtkscrolledwindow.c?ref_type=heads#L173
- https://gitlab.gnome.org/GNOME/gtk/-/blob/filter-constructors/gtk/gtkkineticscrolling.c

# 12-31-2024

### Implement a proper logging system

- https://github.com/gabime/spdlog

### Implement alternative to `std::filesystem`

- https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++-features.md#std_filesystem-banned

### Implement pushing user-defined events

Note that we might have to implement our own event loop separate from the UI one.

- https://developer.apple.com/documentation/appkit/nsevent/eventtype/applicationdefined?language=objc
- https://source.chromium.org/chromium/chromium/src/+/main:third_party/wayland-protocols/gtk/gdk/macos/gdkmacoseventsource.c;l=324?q=otherEventWithType&ss=chromium
- https://developer.apple.com/documentation/appkit/nsevent/otherevent(with:location:modifierflags:timestamp:windownumber:context:subtype:data1:data2:)?language=objc
- https://stackoverflow.com/questions/48020222/how-to-make-nsapp-run-not-block/48023455#48023455
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postmessagea
- https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#application-defined-messages
- https://forum.qt.io/topic/156527/update-ui-from-a-c-library-callback/5?lang=en-US
- https://docs.gtk.org/glib/main-loop.html

### Learn from Chromium's GUI toolkit

- https://chromium.googlesource.com/chromium/src/+/main/docs/ui/create/examples/login_dialog.md
- https://chromium.googlesource.com/chromium/src/+/main/docs/ui/animation_builder/animation_builder.md
- https://chromium.googlesource.com/chromium/src/+/main/docs/ui/input_event/index.md#Focus-Management

### Ensure OpenGL function loading is context-dependent

Apparently, functions are context-dependent on Windows.

- Take inspiration from datenwolf's `struct` approach [here](https://www.reddit.com/r/opengl/comments/17mq767/comment/k7mox6f/)
