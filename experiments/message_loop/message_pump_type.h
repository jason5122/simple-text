#pragma once

#include "build/build_config.h"

namespace base {

// A MessagePump has a particular type, which indicates the set of
// asynchronous events it may process in addition to tasks and timers.

enum class MessagePumpType {
    // This type of pump only supports tasks and timers.
    DEFAULT,

    // This type of pump also supports native UI events (e.g., Windows
    // messages).
    UI,

    // User provided implementation of MessagePump interface
    CUSTOM,

    // This type of pump also supports asynchronous IO.
    IO,

#if BUILDFLAG(IS_MAC)
    // This type of pump is backed by a NSRunLoop.
    NS_RUNLOOP,
#endif
};

}  // namespace base
