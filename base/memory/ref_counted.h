#pragma once

#include "base/compiler_specific.h"
#include "base/memory/atomic_ref_count.h"
#include "base/memory/scoped_refptr.h"
#include <stddef.h>
#include <utility>

namespace base {
namespace subtle {

class RefCountedBase {
public:
    RefCountedBase(const RefCountedBase&) = delete;
    RefCountedBase& operator=(const RefCountedBase&) = delete;

    bool HasOneRef() const { return ref_count_ == 1; }
    bool HasAtLeastOneRef() const { return ref_count_ >= 1; }

protected:
    explicit RefCountedBase(StartRefCountFromZeroTag) {}

    explicit RefCountedBase(StartRefCountFromOneTag) : ref_count_(1) {}

    ~RefCountedBase() {}

    void AddRef() const { ++ref_count_; }

    // Returns true if the object should self-delete.
    bool Release() const {
        --ref_count_;
        return ref_count_ == 0;
    }

    // Returns true if it is safe to read or write the object, from a thread
    // safety standpoint. Should be DCHECK'd from the methods of RefCounted
    // classes if there is a danger of objects being shared across threads.
    //
    // This produces fewer false positives than adding a separate SequenceChecker
    // into the subclass, because it automatically detaches from the sequence when
    // the reference count is 1 (and never fails if there is only one reference).
    //
    // This means unlike a separate SequenceChecker, it will permit a singly
    // referenced object to be passed between threads (not holding a reference on
    // the sending thread), but will trap if the sending thread holds onto a
    // reference, or if the object is accessed from multiple threads
    // simultaneously.
    bool IsOnValidSequence() const { return true; }

private:
    template <typename U>
    friend scoped_refptr<U> base::AdoptRef(U*);

    friend class RefCountedOverflowTest;

    void Adopted() const {}

    mutable uint32_t ref_count_ = 0;
    static_assert(std::is_unsigned_v<decltype(ref_count_)>,
                  "ref_count_ must be an unsigned type.");
};

class RefCountedThreadSafeBase {
public:
    RefCountedThreadSafeBase(const RefCountedThreadSafeBase&) = delete;
    RefCountedThreadSafeBase& operator=(const RefCountedThreadSafeBase&) = delete;

    bool HasOneRef() const { return ref_count_.IsOne(); }
    bool HasAtLeastOneRef() const { return !ref_count_.IsZero(); }

protected:
    explicit constexpr RefCountedThreadSafeBase(StartRefCountFromZeroTag) {}
    explicit constexpr RefCountedThreadSafeBase(StartRefCountFromOneTag) : ref_count_(1) {}

    ~RefCountedThreadSafeBase() = default;

// Release and AddRef are suitable for inlining on X86 because they generate
// very small code sequences.
//
// ARM64 devices supporting ARMv8.1-A atomic instructions generate very little
// code, e.g. fetch_add() with acquire ordering is a single instruction (ldadd),
// vs LL/SC in previous ARM architectures. Inline it there as well.
//
// On other platforms (e.g. ARM), it causes a size regression and is probably
// not worth it.
#if defined(ARCH_CPU_X86_FAMILY) || defined(__ARM_FEATURE_ATOMICS)
    // Returns true if the object should self-delete.
    bool Release() const { return ReleaseImpl(); }
    void AddRef() const { AddRefImpl(); }
    void AddRefWithCheck() const { AddRefWithCheckImpl(); }
#else
    // Returns true if the object should self-delete.
    bool Release() const;
    void AddRef() const;
    void AddRefWithCheck() const;
#endif

private:
    template <typename U>
    friend scoped_refptr<U> base::AdoptRef(U*);

    friend class RefCountedOverflowTest;

    void Adopted() const {}

    ALWAYS_INLINE void AddRefImpl() const { ref_count_.Increment(); }

    ALWAYS_INLINE void AddRefWithCheckImpl() const {
        int pre_increment_count = ref_count_.Increment();
    }

    ALWAYS_INLINE bool ReleaseImpl() const {
        if (!ref_count_.Decrement()) {
            return true;
        }
        return false;
    }

    mutable AtomicRefCount ref_count_{0};
};

}  // namespace subtle

template <typename T>
concept IsRefCountedType = requires(T& x) {
    // There are no additional constraints on `AddRef()` and `Release()` since
    // `scoped_refptr`, for better or worse, seamlessly interoperates with other
    // non-base types that happen to implement the same signatures (e.g. COM's
    // `IUnknown`).
    x.AddRef();
    x.Release();
};

// ScopedAllowCrossThreadRefCountAccess disables the check documented on
// RefCounted below for rare pre-existing use cases where thread-safety was
// guaranteed through other means (e.g. explicit sequencing of calls across
// execution sequences when bouncing between threads in order). New callers
// should refrain from using this (callsites handling thread-safety through
// locks should use RefCountedThreadSafe per the overhead of its atomics being
// negligible compared to locks anyways and callsites doing explicit sequencing
// should properly std::move() the ref to avoid hitting this check).
// TODO(tzik): Cleanup existing use cases and remove
// ScopedAllowCrossThreadRefCountAccess.
class ScopedAllowCrossThreadRefCountAccess final {
public:
    ScopedAllowCrossThreadRefCountAccess() {}
    ~ScopedAllowCrossThreadRefCountAccess() {}
};

//
// A base class for reference counted classes.  Otherwise, known as a cheap
// knock-off of WebKit's RefCounted<T> class.  To use this, just extend your
// class from it like so:
//
//   class MyFoo : public base::RefCounted<MyFoo> {
//    ...
//    private:
//     friend class base::RefCounted<MyFoo>;
//     ~MyFoo();
//   };
//
// Usage Notes:
// 1. You should always make your destructor non-public, to avoid any code
// deleting the object accidentally while there are references to it.
// 2. You should always make the ref-counted base class a friend of your class,
// so that it can access the destructor.
//
// The ref count manipulation to RefCounted is NOT thread safe and has DCHECKs
// to trap unsafe cross thread usage. A subclass instance of RefCounted can be
// passed to another execution sequence only when its ref count is 1. If the ref
// count is more than 1, the RefCounted class verifies the ref updates are made
// on the same execution sequence as the previous ones. The subclass can also
// manually call IsOnValidSequence to trap other non-thread-safe accesses; see
// the documentation for that method.
//
//
// The reference count starts from zero by default, and we intended to migrate
// to start-from-one ref count. Put REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE() to
// the ref counted class to opt-in.
//
// If an object has start-from-one ref count, the first scoped_refptr need to be
// created by base::AdoptRef() or base::MakeRefCounted(). We can use
// base::MakeRefCounted() to create create both type of ref counted object.
//
// The motivations to use start-from-one ref count are:
//  - Start-from-one ref count doesn't need the ref count increment for the
//    first reference.
//  - It can detect an invalid object acquisition for a being-deleted object
//    that has zero ref count. That tends to happen on custom deleter that
//    delays the deletion.
//    TODO(tzik): Implement invalid acquisition detection.
//  - Behavior parity to Blink's WTF::RefCounted, whose count starts from one.
//    And start-from-one ref count is a step to merge WTF::RefCounted into
//    base::RefCounted.
//
#define REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE()                                                    \
    using RefCountPreferenceTag = ::base::subtle::StartRefCountFromOneTag

template <class T, typename Traits>
class RefCounted;

template <typename T>
struct DefaultRefCountedTraits {
    static void Destruct(const T* x) { RefCounted<T, DefaultRefCountedTraits>::DeleteInternal(x); }
};

template <class T, typename Traits = DefaultRefCountedTraits<T>>
class RefCounted : public subtle::RefCountedBase {
public:
    using RefCountPreferenceTag = subtle::StartRefCountFromZeroTag;

    RefCounted() : subtle::RefCountedBase(subtle::GetRefCountPreference<T>()) {}

    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;

    void AddRef() const { subtle::RefCountedBase::AddRef(); }

    void Release() const {
        if (subtle::RefCountedBase::Release()) {
            Traits::Destruct(static_cast<const T*>(this));
        }
    }

protected:
    ~RefCounted() = default;

private:
    friend struct DefaultRefCountedTraits<T>;
    template <typename U>
    static void DeleteInternal(const U* x) {
        delete x;
    }
};

// Forward declaration.
template <class T, typename Traits>
class RefCountedThreadSafe;

// Default traits for RefCountedThreadSafe<T>.  Deletes the object when its ref
// count reaches 0.  Overload to delete it on a different thread etc.
template <typename T>
struct DefaultRefCountedThreadSafeTraits {
    static void Destruct(const T* x) {
        // Delete through RefCountedThreadSafe to make child classes only need to be
        // friend with RefCountedThreadSafe instead of this struct, which is an
        // implementation detail.
        RefCountedThreadSafe<T, DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
    }
};

//
// A thread-safe variant of RefCounted<T>
//
//   class MyFoo : public base::RefCountedThreadSafe<MyFoo> {
//    ...
//   };
//
// If you're using the default trait, then you should add compile time
// asserts that no one else is deleting your object.  i.e.
//    private:
//     friend class base::RefCountedThreadSafe<MyFoo>;
//     ~MyFoo();
//
// We can use REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE() with RefCountedThreadSafe
// too. See the comment above the RefCounted definition for details.
template <class T, typename Traits = DefaultRefCountedThreadSafeTraits<T>>
class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase {
public:
    using RefCountPreferenceTag = subtle::StartRefCountFromZeroTag;

    RefCountedThreadSafe()
        : subtle::RefCountedThreadSafeBase(subtle::GetRefCountPreference<T>()) {}

    RefCountedThreadSafe(const RefCountedThreadSafe&) = delete;
    RefCountedThreadSafe& operator=(const RefCountedThreadSafe&) = delete;

    void AddRef() const { AddRefImpl(subtle::GetRefCountPreference<T>()); }

    void Release() const {
        if (subtle::RefCountedThreadSafeBase::Release()) {
            Traits::Destruct(static_cast<const T*>(this));
        }
    }

protected:
    ~RefCountedThreadSafe() = default;

private:
    friend struct DefaultRefCountedThreadSafeTraits<T>;
    template <typename U>
    static void DeleteInternal(const U* x) {
        delete x;
    }

    void AddRefImpl(subtle::StartRefCountFromZeroTag) const {
        subtle::RefCountedThreadSafeBase::AddRef();
    }

    void AddRefImpl(subtle::StartRefCountFromOneTag) const {
        subtle::RefCountedThreadSafeBase::AddRefWithCheck();
    }
};

//
// A thread-safe wrapper for some piece of data so we can place other
// things in scoped_refptrs<>.
//
template <typename T>
class RefCountedData : public base::RefCountedThreadSafe<base::RefCountedData<T>> {
public:
    RefCountedData() : data() {}
    RefCountedData(const T& in_value) : data(in_value) {}
    RefCountedData(T&& in_value) : data(std::move(in_value)) {}
    template <typename... Args>
    explicit RefCountedData(std::in_place_t, Args&&... args) : data(std::forward<Args>(args)...) {}

    T data;

private:
    friend class base::RefCountedThreadSafe<base::RefCountedData<T>>;
    ~RefCountedData() = default;
};

template <typename T>
bool operator==(const RefCountedData<T>& lhs, const RefCountedData<T>& rhs) {
    return lhs.data == rhs.data;
}

}  // namespace base
