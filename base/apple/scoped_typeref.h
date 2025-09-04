#pragma once

#include <cstddef>

namespace base::apple {

enum class OwnershipPolicy { kAssume, kRetain };

template <typename T>
struct ScopedTypeRefTraits;

template <typename T, typename Traits = ScopedTypeRefTraits<T>>
class ScopedTypeRef {
public:
    using element_type = T;

    // Construction from underlying type

    explicit constexpr ScopedTypeRef(element_type object = Traits::InvalidValue(),
                                     OwnershipPolicy policy = OwnershipPolicy::kAssume)
        : object_(object) {
        if (object_ != Traits::InvalidValue() && policy == OwnershipPolicy::kRetain) {
            object_ = Traits::Retain(object_);
        }
    }

    // The pattern in the four [copy|move] [constructors|assignment operators]
    // below is that for each of them there is the standard version for use by
    // scopers wrapping objects of this type, and a templated version to handle
    // scopers wrapping objects of subtypes. One might think that one could get
    // away only the templated versions, as their templates should match the
    // usage, but that doesn't work. Having a templated function that matches the
    // types of, say, a copy constructor, doesn't count as a copy constructor, and
    // the compiler's generated copy constructor is incorrect.

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
        reset(that.get(), OwnershipPolicy::kRetain);
        return *this;
    }

    template <typename R, typename RTraits>
    ScopedTypeRef& operator=(const ScopedTypeRef<R, RTraits>& that) {
        reset(that.get(), OwnershipPolicy::kRetain);
        return *this;
    }

    // Move construction

    ScopedTypeRef(ScopedTypeRef<T, Traits>&& that) : object_(that.release()) {}

    template <typename R, typename RTraits>
    ScopedTypeRef(ScopedTypeRef<R, RTraits>&& that) : object_(that.release()) {}

    // Move assignment

    ScopedTypeRef& operator=(ScopedTypeRef<T, Traits>&& that) {
        reset(that.release(), OwnershipPolicy::kAssume);
        return *this;
    }

    template <typename R, typename RTraits>
    ScopedTypeRef& operator=(ScopedTypeRef<R, RTraits>&& that) {
        reset(that.release(), OwnershipPolicy::kAssume);
        return *this;
    }

    // Resetting

    template <typename R, typename RTraits>
    void reset(const ScopedTypeRef<R, RTraits>& that) {
        reset(that.get(), OwnershipPolicy::kRetain);
    }

    void reset(element_type object = Traits::InvalidValue(),
               OwnershipPolicy policy = OwnershipPolicy::kAssume) {
        if (object != Traits::InvalidValue() && policy == OwnershipPolicy::kRetain) {
            object = Traits::Retain(object);
        }
        if (object_ != Traits::InvalidValue()) {
            Traits::Release(object_);
        }
        object_ = object;
    }

    // Destruction

    ~ScopedTypeRef() {
        if (object_ != Traits::InvalidValue()) {
            Traits::Release(object_);
        }
    }

    // This is to be used only to take ownership of objects that are created by
    // pass-by-pointer create functions. To enforce this, require that this object
    // be empty before use.
    [[nodiscard]] element_type* InitializeInto() {
        CHECK_EQ(object_, Traits::InvalidValue());
        return &object_;
    }

    bool operator==(const ScopedTypeRef& that) const { return object_ == that.object_; }

    bool operator!=(const ScopedTypeRef& that) const { return object_ != that.object_; }

    explicit operator bool() const { return object_ != Traits::InvalidValue(); }

    element_type get() const { return object_; }

    void swap(ScopedTypeRef& that) {
        element_type temp = that.object_;
        that.object_ = object_;
        object_ = temp;
    }

    // ScopedTypeRef<>::release() is like std::unique_ptr<>::release.  It is NOT
    // a wrapper for Release().  To force a ScopedTypeRef<> object to call
    // Release(), use ScopedTypeRef<>::reset().
    [[nodiscard]] element_type release() {
        element_type temp = object_;
        object_ = Traits::InvalidValue();
        return temp;
    }

private:
    element_type object_;
};

}  // namespace base::apple
