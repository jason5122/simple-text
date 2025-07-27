#pragma once

// Weak pointers are pointers to an object that do not affect its lifetime,
// and which may be invalidated (i.e. reset to nullptr) by the object, or its
// owner, at any time, most commonly when the object is about to be deleted.

// Weak pointers are useful when an object needs to be accessed safely by one
// or more objects other than its owner, and those callers can cope with the
// object vanishing and e.g. tasks posted to it being silently dropped.
// Reference-counting such an object would complicate the ownership graph and
// make it harder to reason about the object's lifetime.

// EXAMPLE:
//
//  class Controller {
//   public:
//    void SpawnWorker() { Worker::StartNew(weak_factory_.GetWeakPtr()); }
//    void WorkComplete(const Result& result) { ... }
//   private:
//    // Member variables should appear before the WeakPtrFactory, to ensure
//    // that any WeakPtrs to Controller are invalidated before its members
//    // variable's destructors are executed, rendering them invalid.
//    WeakPtrFactory<Controller> weak_factory_{this};
//  };
//
//  class Worker {
//   public:
//    static void StartNew(WeakPtr<Controller> controller) {
//      // Move WeakPtr when possible to avoid atomic refcounting churn on its
//      // internal state.
//      Worker* worker = new Worker(std::move(controller));
//      // Kick off asynchronous processing...
//    }
//   private:
//    Worker(WeakPtr<Controller> controller)
//        : controller_(std::move(controller)) {}
//    void DidCompleteAsynchronousProcessing(const Result& result) {
//      if (controller_)
//        controller_->WorkComplete(result);
//    }
//    WeakPtr<Controller> controller_;
//  };
//
// With this implementation a caller may use SpawnWorker() to dispatch multiple
// Workers and subsequently delete the Controller, without waiting for all
// Workers to have completed.

#include "base/memory/atomic_flag.h"
#include "base/memory/ref_counted.h"
#include <cstddef>
#include <type_traits>
#include <utility>

namespace base {

template <typename T>
class WeakPtr;

namespace internal {
// These classes are part of the WeakPtr implementation.
// DO NOT USE THESE CLASSES DIRECTLY YOURSELF.

class WeakReference {
public:
    // Although Flag is bound to a specific SequencedTaskRunner, it may be
    // deleted from another via base::WeakPtr::~WeakPtr().
    class Flag : public RefCountedThreadSafe<Flag> {
    public:
        Flag() = default;

        void Invalidate() { invalidated_.Set(); }
        bool IsValid() const { return !invalidated_.IsSet(); }
        bool MaybeValid() const { return !invalidated_.IsSet(); }

    private:
        friend class base::RefCountedThreadSafe<Flag>;

        ~Flag() = default;

        AtomicFlag invalidated_;
    };

    WeakReference() = default;
    explicit WeakReference(const scoped_refptr<Flag>& flag) : flag_(flag) {}
    ~WeakReference() = default;

    WeakReference(const WeakReference& other) = default;
    WeakReference& operator=(const WeakReference& other) = default;
    WeakReference(WeakReference&& other) noexcept = default;
    WeakReference& operator=(WeakReference&& other) noexcept = default;

    void Reset() { flag_ = nullptr; }
    // Returns whether the WeakReference is valid, meaning the WeakPtrFactory has
    // not invalidated the pointer. Unlike, RefIsMaybeValid(), this may only be
    // called from the same sequence as where the WeakPtr was created.
    bool IsValid() const { return flag_ && flag_->IsValid(); }
    // Returns false if the WeakReference is confirmed to be invalid. This call is
    // safe to make from any thread, e.g. to optimize away unnecessary work, but
    // RefIsValid() must always be called, on the correct sequence, before
    // actually using the pointer.
    //
    // Warning: as with any object, this call is only thread-safe if the WeakPtr
    // instance isn't being re-assigned or reset() racily with this call.
    bool MaybeValid() const { return flag_ && flag_->MaybeValid(); }

private:
    scoped_refptr<const Flag> flag_;
};

class WeakReferenceOwner {
public:
    WeakReferenceOwner() : flag_(MakeRefCounted<WeakReference::Flag>()) {}
    ~WeakReferenceOwner() {
        if (flag_) {
            flag_->Invalidate();
        }
    }

    WeakReference GetRef() const { return WeakReference(flag_); }

    bool HasRefs() const { return !flag_->HasOneRef(); }

    void Invalidate() {
        flag_->Invalidate();
        flag_ = MakeRefCounted<WeakReference::Flag>();
    }
    void InvalidateAndDoom() {
        flag_->Invalidate();
        flag_.reset();
    }

private:
    scoped_refptr<WeakReference::Flag> flag_;
};

}  // namespace internal

template <typename T>
class WeakPtrFactory;

// The WeakPtr class holds a weak reference to |T*|.
//
// This class is designed to be used like a normal pointer.  You should always
// null-test an object of this class before using it or invoking a method that
// may result in the underlying object being destroyed.
//
// EXAMPLE:
//
//   class Foo { ... };
//   WeakPtr<Foo> foo;
//   if (foo)
//     foo->method();
//
// WeakPtr intentionally doesn't implement operator== or operator<=>, because
// comparisons of weak references are inherently unstable. If the comparison
// takes validity into account, the result can change at any time as pointers
// are invalidated. If it depends only on the underlying pointer value, even
// after the pointer is invalidated, unrelated WeakPtrs can unexpectedly
// compare equal if the address is reused.
template <typename T>
class WeakPtr {
public:
    WeakPtr() = default;
    WeakPtr(std::nullptr_t) {}

    // Allow conversion from U to T provided U "is a" T. Note that this
    // is separate from the (implicit) copy and move constructors.
    template <typename U>
        requires(std::convertible_to<U*, T*>)
    WeakPtr(const WeakPtr<U>& other) : ref_(other.ref_), ptr_(other.ptr_) {}
    template <typename U>
        requires(std::convertible_to<U*, T*>)
    WeakPtr& operator=(const WeakPtr<U>& other) {
        ref_ = other.ref_;
        ptr_ = other.ptr_;
        return *this;
    }

    template <typename U>
        requires(std::convertible_to<U*, T*>)
    WeakPtr(WeakPtr<U>&& other) : ref_(std::move(other.ref_)), ptr_(std::move(other.ptr_)) {}
    template <typename U>
        requires(std::convertible_to<U*, T*>)
    WeakPtr& operator=(WeakPtr<U>&& other) {
        ref_ = std::move(other.ref_);
        ptr_ = std::move(other.ptr_);
        return *this;
    }

    T* get() const { return ref_.IsValid() ? ptr_ : nullptr; }

    // Provide access to the underlying T as a reference.
    T& operator*() const { return *ptr_; }

    // Used to call methods on the underlying T.
    T* operator->() const { return ptr_; }

    // Allow conditionals to test validity, e.g. if (weak_ptr) {...};
    explicit operator bool() const { return get() != nullptr; }

    // Resets the WeakPtr to hold nothing.
    //
    // The `get()` method will return `nullptr` thereafter, and `MaybeValid()`
    // will be `false`.
    void reset() {
        ref_.Reset();
        ptr_ = nullptr;
    }

    // Returns whether the object |this| points to has been invalidated. This can
    // be used to distinguish a WeakPtr to a destroyed object from one that has
    // been explicitly set to null.
    bool WasInvalidated() const { return ptr_ && !ref_.IsValid(); }

private:
    template <typename U>
    friend class WeakPtr;
    friend class WeakPtrFactory<T>;
    friend class WeakPtrFactory<std::remove_const_t<T>>;

    WeakPtr(internal::WeakReference&& ref, T* ptr) : ref_(std::move(ref)), ptr_(ptr) {}

    internal::WeakReference CloneWeakReference() const { return ref_; }

    internal::WeakReference ref_;

    // This pointer is only valid when ref_.is_valid() is true.  Otherwise, its
    // value is undefined (as opposed to nullptr). The pointer is allowed to
    // dangle as we verify its liveness through `ref_` before allowing access to
    // the pointee. We don't use raw_ptr<T> here to prevent WeakPtr from keeping
    // the memory allocation in quarantine, as it can't be accessed through the
    // WeakPtr.
    T* ptr_ = nullptr;
};

// Allow callers to compare WeakPtrs against nullptr to test validity.
template <class T>
bool operator!=(const WeakPtr<T>& weak_ptr, std::nullptr_t) {
    return !(weak_ptr == nullptr);
}
template <class T>
bool operator!=(std::nullptr_t, const WeakPtr<T>& weak_ptr) {
    return weak_ptr != nullptr;
}
template <class T>
bool operator==(const WeakPtr<T>& weak_ptr, std::nullptr_t) {
    return weak_ptr.get() == nullptr;
}
template <class T>
bool operator==(std::nullptr_t, const WeakPtr<T>& weak_ptr) {
    return weak_ptr == nullptr;
}

namespace internal {
class WeakPtrFactoryBase {
protected:
    WeakPtrFactoryBase(uintptr_t ptr) : ptr_(ptr) {}
    ~WeakPtrFactoryBase() { ptr_ = 0; }
    internal::WeakReferenceOwner weak_reference_owner_;
    uintptr_t ptr_;
};
}  // namespace internal

// A class may be composed of a WeakPtrFactory and thereby
// control how it exposes weak pointers to itself.  This is helpful if you only
// need weak pointers within the implementation of a class.  This class is also
// useful when working with primitive types.  For example, you could have a
// WeakPtrFactory<bool> that is used to pass around a weak reference to a bool.
template <class T>
class WeakPtrFactory : public internal::WeakPtrFactoryBase {
public:
    explicit WeakPtrFactory(T* ptr) : WeakPtrFactoryBase(reinterpret_cast<uintptr_t>(ptr)) {}
    ~WeakPtrFactory() = default;
    WeakPtrFactory(const WeakPtrFactory&) = delete;
    WeakPtrFactory& operator=(const WeakPtrFactory&) = delete;

    WeakPtr<const T> GetWeakPtr() const {
        return WeakPtr<const T>(weak_reference_owner_.GetRef(), reinterpret_cast<const T*>(ptr_));
    }

    WeakPtr<T> GetWeakPtr()
        requires(!std::is_const_v<T>)
    {
        return WeakPtr<T>(weak_reference_owner_.GetRef(), reinterpret_cast<T*>(ptr_));
    }

    WeakPtr<T> GetMutableWeakPtr() const
        requires(!std::is_const_v<T>)
    {
        return WeakPtr<T>(weak_reference_owner_.GetRef(), reinterpret_cast<T*>(ptr_));
    }

    // Invalidates all existing weak pointers.
    void InvalidateWeakPtrs() { weak_reference_owner_.Invalidate(); }

    // Invalidates all existing weak pointers, and makes the factory unusable
    // (cannot call GetWeakPtr after this). This is more efficient than
    // InvalidateWeakPtrs().
    void InvalidateWeakPtrsAndDoom() {
        weak_reference_owner_.InvalidateAndDoom();
        ptr_ = 0;
    }

    // Call this method to determine if any weak pointers exist.
    bool HasWeakPtrs() const { return ptr_ && weak_reference_owner_.HasRefs(); }
};

}  // namespace base
