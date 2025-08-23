#pragma once

#include <memory>
#include <string>
#if __OBJC__
#include <AppKit/AppKit.h>
#endif

namespace base::apple {

class OwnedNSEvent {
public:
    OwnedNSEvent();
    ~OwnedNSEvent();
    OwnedNSEvent(const OwnedNSEvent&);
    OwnedNSEvent& operator=(const OwnedNSEvent&);

    explicit operator bool() const;
    bool operator==(const OwnedNSEvent& other) const;
    std::string to_string() const;

#if __OBJC__
    explicit OwnedNSEvent(NSEvent* event);
    NSEvent* get() const;
#endif

private:
    struct ObjCStorage;
    std::unique_ptr<ObjCStorage> objc_storage_;
};

}  // namespace base::apple
