#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

// Delivers recorded input timestamps independently of the display clock. The macOS implementation
// waits on mach continuous time before hopping to the main queue, avoiding dispatch timer leeway
// that would otherwise turn adjacent 120 Hz samples into artificial bursts.
//
// The callback receives the time the sample was scheduled for, on the px_now() clock. That plays
// the part of a native event's own timestamp: the time the main thread got to it is jittered by
// whatever the thread was doing.
void schedule_timed_input(const std::vector<uint64_t>& timestamps_ns,
                          uint64_t phase_ns,
                          std::function<void(size_t index, double scheduled_time)> callback);
