// Every wrapper is 32 bytes: a pointer to a per-callable table of static functions, then a
// 24-byte buffer. Callables of 24 bytes or less with pointer alignment live in the buffer; larger
// ones are heap-allocated and the buffer holds the pointer. The table pointer doubles as the
// empty flag. There are no virtual functions: each function_value<F> instantiation builds its
// own table as a function-local static inside make().

#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace base {

namespace internal {

// The 24 bytes after the table pointer. Read as a pointer when the callable is on the heap.
union function_storage {
    void* heap;
    alignas(void*) unsigned char local[3 * sizeof(void*)];
};

// Where a callable of type F lives and how it gets there: in the buffer when it fits, otherwise
// on the heap with the buffer holding the pointer.
template <class F>
struct stored {
    static constexpr bool kInline =
        sizeof(F) <= sizeof(function_storage) && alignof(F) <= alignof(function_storage);

    static F* get(function_storage& storage) {
        if constexpr (kInline) return reinterpret_cast<F*>(storage.local);
        else return static_cast<F*>(storage.heap);
    }

    static const F* get(const function_storage& storage) {
        if constexpr (kInline) return reinterpret_cast<const F*>(storage.local);
        else return static_cast<const F*>(storage.heap);
    }

    static void construct(function_storage& storage, F&& f) {
        if constexpr (kInline) new (storage.local) F(std::move(f));
        else storage.heap = new F(std::move(f));
    }

    static void copy(function_storage& dst, const function_storage& src) {
        if constexpr (kInline) new (dst.local) F(*get(src));
        else dst.heap = new F(*get(src));
    }

    // Moves the callable across and destroys the source. A heap callable only changes owner; the
    // caller forgets the source afterwards by clearing its table pointer.
    static void move(function_storage& dst, function_storage& src) {
        if constexpr (kInline) {
            new (dst.local) F(std::move(*get(src)));
            get(src)->~F();
        } else {
            dst.heap = src.heap;
        }
    }

    static void destroy(function_storage& storage) {
        if constexpr (kInline) get(storage)->~F();
        else delete get(storage);
    }
};

}  // namespace internal

template <class Sig>
class function;

template <class Sig>
class base_function;

// Storage and lifetime shared by function<Sig> and function<Sig &&>. Not callable by itself.
template <class R, class... Args>
class base_function<R(Args...)> {
public:
    base_function() = default;

    // Takes the callable by value: every make() in the binary receives an rvalue.
    template <class F>
        requires(!std::is_base_of_v<base_function, F> && std::is_invocable_r_v<R, F&, Args...>)
    base_function(F f) {
        function_value<F>::make(this, std::move(f));
    }

    base_function(const base_function&) = delete;
    base_function& operator=(const base_function&) = delete;

    base_function(base_function&& other) noexcept : vt_(other.vt_) {
        if (vt_) {
            vt_->move(this, &other);
            other.vt_ = nullptr;
        }
    }

    base_function& operator=(base_function&& other) noexcept {
        if (this != &other) {
            if (vt_) vt_->dtor(this);
            vt_ = other.vt_;
            if (vt_) {
                vt_->move(this, &other);
                other.vt_ = nullptr;
            }
        }
        return *this;
    }

    ~base_function() {
        if (vt_) vt_->dtor(this);
    }

    explicit operator bool() const { return vt_ != nullptr; }

protected:
    // One table per callable type, filled with function_value<F>'s static functions.
    struct vtable {
        R (*call)(base_function* self, Args... args);
        void (*move)(base_function* dst, base_function* src);
        void (*dtor)(base_function* self);
    };

    template <class F>
    struct function_value;

    const vtable* vt_ = nullptr;
    internal::function_storage storage_;
};

template <class R, class... Args>
template <class F>
struct base_function<R(Args...)>::function_value {
    using stored = internal::stored<F>;

    static R call(base_function* self, Args... args) {
        return (*stored::get(self->storage_))(std::forward<Args>(args)...);
    }

    static void move(base_function* dst, base_function* src) {
        stored::move(dst->storage_, src->storage_);
    }

    static void dtor(base_function* self) { stored::destroy(self->storage_); }

    static void make(base_function* self, F&& f) {
        static const vtable vt = {&call, &move, &dtor};
        stored::construct(self->storage_, std::move(f));
        self->vt_ = &vt;
    }
};

// The move-only callable. Calling leaves the callable in place. Calling an empty function is
// undefined; there is no check.
template <class R, class... Args>
class function<R(Args...)> : public base_function<R(Args...)> {
    using base = base_function<R(Args...)>;

public:
    using base::base;

    // Non-const, unlike Sublime's, so a mutable callable runs without a const_cast.
    R operator()(Args... args) { return this->vt_->call(this, std::forward<Args>(args)...); }
};

// The call-once callable. Calling moves the callable out, runs it, and destroys it on return, so
// the function is empty afterwards.
template <class R, class... Args>
class function<R(Args...) &&> : public base_function<R(Args...)> {
    using base = base_function<R(Args...)>;

public:
    using base::base;

    R operator()(Args... args) && {
        function f(std::move(*this));
        return f.vt_->call(&f, std::forward<Args>(args)...);
    }
};

template <class Sig>
class copyable_function;

// The copyable sibling: the same layout with a fourth table slot for copying.
template <class R, class... Args>
class copyable_function<R(Args...)> {
public:
    copyable_function() = default;

    template <class F>
        requires(!std::is_same_v<F, copyable_function> && std::is_invocable_r_v<R, F&, Args...>)
    copyable_function(F f) {
        function_value<F>::make(this, std::move(f));
    }

    copyable_function(const copyable_function& other) : vt_(other.vt_) {
        if (vt_) vt_->copy(this, &other);
    }

    copyable_function(copyable_function&& other) noexcept : vt_(other.vt_) {
        if (vt_) {
            vt_->move(this, &other);
            other.vt_ = nullptr;
        }
    }

    copyable_function& operator=(const copyable_function& other) {
        if (this != &other) {
            if (vt_) vt_->dtor(this);
            vt_ = other.vt_;
            if (vt_) vt_->copy(this, &other);
        }
        return *this;
    }

    copyable_function& operator=(copyable_function&& other) noexcept {
        if (this != &other) {
            if (vt_) vt_->dtor(this);
            vt_ = other.vt_;
            if (vt_) {
                vt_->move(this, &other);
                other.vt_ = nullptr;
            }
        }
        return *this;
    }

    ~copyable_function() {
        if (vt_) vt_->dtor(this);
    }

    explicit operator bool() const { return vt_ != nullptr; }

    R operator()(Args... args) { return vt_->call(this, std::forward<Args>(args)...); }

private:
    struct vtable {
        R (*call)(copyable_function* self, Args... args);
        void (*move)(copyable_function* dst, copyable_function* src);
        void (*dtor)(copyable_function* self);
        void (*copy)(copyable_function* dst, const copyable_function* src);
    };

    template <class F>
    struct function_value;

    const vtable* vt_ = nullptr;
    internal::function_storage storage_;
};

template <class R, class... Args>
template <class F>
struct copyable_function<R(Args...)>::function_value {
    using stored = internal::stored<F>;

    static R call(copyable_function* self, Args... args) {
        return (*stored::get(self->storage_))(std::forward<Args>(args)...);
    }

    static void move(copyable_function* dst, copyable_function* src) {
        stored::move(dst->storage_, src->storage_);
    }

    static void dtor(copyable_function* self) { stored::destroy(self->storage_); }

    static void copy(copyable_function* dst, const copyable_function* src) {
        stored::copy(dst->storage_, src->storage_);
    }

    static void make(copyable_function* self, F&& f) {
        static const vtable vt = {&call, &move, &dtor, &copy};
        stored::construct(self->storage_, std::move(f));
        self->vt_ = &vt;
    }
};

template <class Sig>
class function_ref;

template <class R, class... Args>
class function_ref<R(Args...)> {
public:
    template <class F>
        requires(!std::is_same_v<std::remove_cvref_t<F>, function_ref> &&
                 std::is_invocable_r_v<R, F&, Args...>)
    function_ref(F&& f)
        : call_(&call<std::remove_reference_t<F>>), object_(reinterpret_cast<std::intptr_t>(&f)) {}

    R operator()(Args... args) const { return call_(object_, std::forward<Args>(args)...); }

private:
    template <class F>
    static R call(std::intptr_t object, Args... args) {
        return (*reinterpret_cast<F*>(object))(std::forward<Args>(args)...);
    }

    R (*call_)(std::intptr_t, Args...);
    std::intptr_t object_;
};

}  // namespace base
