#pragma once

#import <Cocoa/Cocoa.h>

namespace ui {

// Converts the Cocoa |modifiers| bitsum into a ui::EventFlags bitsum.
int EventFlagsFromModifiers(NSUInteger modifiers);

// Retrieves a bitsum of ui::EventFlags represented by |event|, but instead use the modifier flags
// given by |modifiers|, which is the same format as |-NSEvent modifierFlags|. This allows
// substitution of the modifiers without having to create a new event from scratch.
int EventFlagsFromNSEventWithModifiers(NSEvent* event, NSUInteger modifiers);

bool IsKeyUpEvent(NSEvent* event);

}  // namespace ui
