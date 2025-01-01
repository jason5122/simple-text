# TODO

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
