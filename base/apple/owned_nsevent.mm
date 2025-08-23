#include "base/apple/owned_nsevent.h"
#include <Foundation/Foundation.h>

namespace base::apple {

struct OwnedNSEvent::ObjCStorage {
    NSEvent* __strong obj;
};

OwnedNSEvent::OwnedNSEvent() : objc_storage_(std::make_unique<ObjCStorage>()) {}

OwnedNSEvent::~OwnedNSEvent() = default;

OwnedNSEvent::OwnedNSEvent(NSEvent* obj) : OwnedNSEvent() { objc_storage_->obj = obj; }

OwnedNSEvent::OwnedNSEvent(const OwnedNSEvent& other) : OwnedNSEvent() {
    objc_storage_->obj = other.objc_storage_->obj;
}

OwnedNSEvent& OwnedNSEvent::operator=(const OwnedNSEvent& other) {
    objc_storage_->obj = other.objc_storage_->obj;
    return *this;
}

OwnedNSEvent::operator bool() const { return objc_storage_->obj != nil; }

bool OwnedNSEvent::operator==(const OwnedNSEvent& other) const {
    return objc_storage_->obj == other.objc_storage_->obj;
}

std::string OwnedNSEvent::to_string() const {
    return objc_storage_->obj ? id<NSObject>(objc_storage_->obj).debugDescription.UTF8String
                              : std::string("<nil>");
}

NSEvent* OwnedNSEvent::get() const { return objc_storage_->obj; }

}  // namespace base::apple
