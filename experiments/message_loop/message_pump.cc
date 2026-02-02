#include "build/build_config.h"
#include "experiments/message_loop/message_pump.h"

#if BUILDFLAG(IS_MAC)
#include "experiments/message_loop/message_pump_mac.h"
#endif

MessagePump::MessagePump() = default;

MessagePump::~MessagePump() = default;

std::unique_ptr<MessagePump> MessagePump::create() {
#if BUILDFLAG(IS_MAC)
    return message_pump_mac::create();
#endif
    NOTREACHED();
}
