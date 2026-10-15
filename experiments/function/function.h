// Every wrapper is 32 bytes: a pointer to a per-callable table of static functions, then a
// 24-byte buffer. Callables of 24 bytes or less with pointer alignment live in the buffer; larger
// ones are heap-allocated and the buffer holds the pointer. The table pointer doubles as the
// empty flag. There are no virtual functions: each function_value<F> instantiation builds its
// own table as a function-local static inside make().

#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace base {

namespace internal {

// The 24 bytes after the table pointer. Read as a pointer when the callable is on the heap.
union function_storage {
    void* heap;
    alignas(void*) unsigned char local[3 * sizeof(void*)];
};

template <class F>
inline constexpr bool kInlineCallable =
    sizeof(F) <= sizeof(function_storage) && alignof(F) <= alignof(function_storage);

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

    // Destroys the current callable first. There is no self-move guard.
    base_function& operator=(base_function&& other) noexcept {
        if (vt_) vt_->dtor(this);
        vt_ = other.vt_;
        if (vt_) {
            vt_->move(this, &other);
            other.vt_ = nullptr;
        }
        return *this;
    }

    ~base_function() {
        if (vt_) vt_->dtor(this);
    }

    explicit operator bool() const { return vt_ != nullptr; }

protected:
    template <class>
    friend class function;

    struct vtable {
        R (*call)(const base_function* self, Args... args);
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
    static constexpr bool kInline = internal::kInlineCallable<F>;

    static F* get(const base_function* self) {
        auto* storage = const_cast<internal::function_storage*>(&self->storage_);
        if constexpr (kInline) return reinterpret_cast<F*>(storage->local);
        else return static_cast<F*>(storage->heap);
    }

    static R call(const base_function* self, Args... args) {
        return (*get(self))(std::forward<Args>(args)...);
    }

    // Moves the callable across and destroys the source; the caller clears the source's table
    // pointer. A heap callable only changes owner.
    static void move(base_function* dst, base_function* src) {
        if constexpr (kInline) {
            new (dst->storage_.local) F(std::move(*get(src)));
            get(src)->~F();
        } else {
            dst->storage_.heap = src->storage_.heap;
        }
    }

    static void dtor(base_function* self) {
        if constexpr (kInline) get(self)->~F();
        else delete get(self);
    }

    static void make(base_function* self, F&& f) {
        static const vtable vt = {&call, &move, &dtor};
        if constexpr (kInline) new (self->storage_.local) F(std::move(f));
        else self->storage_.heap = new F(std::move(f));
        self->vt_ = &vt;
    }
};

// The move-only callable. Calling is const and leaves the callable in place. Calling an empty
// function is undefined; there is no check.
template <class R, class... Args>
class function<R(Args...)> : public base_function<R(Args...)> {
    using base = base_function<R(Args...)>;

public:
    using base::base;

    R operator()(Args... args) const { return this->vt_->call(this, std::forward<Args>(args)...); }
};

// The call-once callable. Calling moves the callable onto the stack, runs it, and destroys it on
// return, so the function is empty afterwards.
template <class R, class... Args>
class function<R(Args...) &&> : public base_function<R(Args...)> {
    using base = base_function<R(Args...)>;

public:
    using base::base;

    R operator()(Args... args) && {
        base f(std::move(*this));
        return f.vt_->call(&f, std::forward<Args>(args)...);
    }
};

template <class Sig>
class copyable_function;

// The copyable sibling: the same layout with a fourth table slot for copying. Calling is
// non-const.
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

    // Copy assignment guards against self-assignment; move assignment does not.
    copyable_function& operator=(const copyable_function& other) {
        if (this != &other) {
            if (vt_) vt_->dtor(this);
            vt_ = other.vt_;
            if (vt_) vt_->copy(this, &other);
        }
        return *this;
    }

    copyable_function& operator=(copyable_function&& other) noexcept {
        if (vt_) vt_->dtor(this);
        vt_ = other.vt_;
        if (vt_) {
            vt_->move(this, &other);
            other.vt_ = nullptr;
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
    static constexpr bool kInline = internal::kInlineCallable<F>;

    static F* get(const copyable_function* self) {
        auto* storage = const_cast<internal::function_storage*>(&self->storage_);
        if constexpr (kInline) return reinterpret_cast<F*>(storage->local);
        else return static_cast<F*>(storage->heap);
    }

    static R call(copyable_function* self, Args... args) {
        return (*get(self))(std::forward<Args>(args)...);
    }

    static void move(copyable_function* dst, copyable_function* src) {
        if constexpr (kInline) {
            new (dst->storage_.local) F(std::move(*get(src)));
            get(src)->~F();
        } else {
            dst->storage_.heap = src->storage_.heap;
        }
    }

    static void dtor(copyable_function* self) {
        if constexpr (kInline) get(self)->~F();
        else delete get(self);
    }

    static void copy(copyable_function* dst, const copyable_function* src) {
        if constexpr (kInline) new (dst->storage_.local) F(*get(src));
        else dst->storage_.heap = new F(*get(src));
    }

    static void make(copyable_function* self, F&& f) {
        static const vtable vt = {&call, &move, &dtor, &copy};
        if constexpr (kInline) new (self->storage_.local) F(std::move(f));
        else self->storage_.heap = new F(std::move(f));
        self->vt_ = &vt;
    }
};

}  // namespace base
