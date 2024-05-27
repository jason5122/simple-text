#pragma once

#include <cstddef>

namespace base::apple {

// Defines the ownership policy for a scoped object.
enum class OwnershipPolicy {
    // The scoped object takes ownership of an object by taking over an existing
    // ownership claim.
    ASSUME,

    // The scoped object will retain the object and any initial ownership is
    // not changed.
    RETAIN
};

template <typename T> struct ScopedTypeRefTraits;

template <typename T, typename Traits = ScopedTypeRefTraits<T>> class ScopedTypeRef {
public:
    using element_type = T;

    // explicit constexpr ScopedTypeRef(element_type object = Traits::InvalidValue())
    //     : object_(object) {}
    constexpr ScopedTypeRef(element_type object = Traits::InvalidValue()) : object_(object) {}

    // Copy construction
    ScopedTypeRef(const ScopedTypeRef<T, Traits>& that) : object_(that.get()) {
        if (object_ != Traits::InvalidValue()) {
            object_ = Traits::Retain(object_);
        }
    }

    template <typename R, typename RTraits>
    ScopedTypeRef(const ScopedTypeRef<R, RTraits>& that) : object_(that.get()) {
        if (object_ != Traits::InvalidValue()) {
            object_ = Traits::Retain(object_);
        }
    }

    // Copy assignment
    ScopedTypeRef& operator=(const ScopedTypeRef<T, Traits>& that) {
        reset(that.get(), OwnershipPolicy::RETAIN);
        return *this;
    }

    template <typename R, typename RTraits>
    ScopedTypeRef& operator=(const ScopedTypeRef<R, RTraits>& that) {
        reset(that.get(), OwnershipPolicy::RETAIN);
        return *this;
    }

    // Move construction
    ScopedTypeRef(ScopedTypeRef<T, Traits>&& that) : object_(that.release()) {}

    template <typename R, typename RTraits>
    ScopedTypeRef(ScopedTypeRef<R, RTraits>&& that) : object_(that.release()) {}

    // Move assignment
    ScopedTypeRef& operator=(ScopedTypeRef<T, Traits>&& that) {
        reset(that.release(), OwnershipPolicy::ASSUME);
        return *this;
    }

    template <typename R, typename RTraits>
    ScopedTypeRef& operator=(ScopedTypeRef<R, RTraits>&& that) {
        reset(that.release(), OwnershipPolicy::ASSUME);
        return *this;
    }

    void reset(element_type object = Traits::InvalidValue(),
               OwnershipPolicy policy = OwnershipPolicy::RETAIN) {
        if (object != Traits::InvalidValue() && policy == OwnershipPolicy::RETAIN) {
            object = Traits::Retain(object);
        }
        if (object_ != Traits::InvalidValue()) {
            Traits::Release(object_);
        }
        object_ = object;
    }

    ~ScopedTypeRef() {
        if (object_ != Traits::InvalidValue()) {
            Traits::Release(object_);
        }
    }

    bool operator==(const ScopedTypeRef& that) const {
        return object_ == that.object_;
    }

    bool operator!=(const ScopedTypeRef& that) const {
        return object_ != that.object_;
    }

    element_type* operator&() {
        return &object_;
    }

    explicit operator bool() const {
        return object_ != Traits::InvalidValue();
    }

    element_type get() const {
        return object_;
    }

private:
    element_type object_;
};

template <typename T, typename Traits = ScopedTypeRefTraits<T>>
bool operator==(const T& that, std::nullptr_t) {
    return false;
}

}
