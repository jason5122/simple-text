#pragma once

#include <memory>

namespace base {

namespace {
// Make sure that the object is not destroyed by the shared_ptr.
template <class T>
void mock_deleter(T*) {}
}  // namespace

// Creates weak_ptrs (mainly) for self managing object.
// Idea taken from Chromium's base::WeakPtrFactory.
template <class T>
class weak_ptr_factory {
    std::shared_ptr<T> ptr_;

public:
    // Initializes factory with `this`.
    explicit weak_ptr_factory(T* instance) : ptr_(instance, mock_deleter<T>) {}

    std::weak_ptr<T> get_weak_ptr() const {
        return ptr_;
    }

    void invalidate_all_ptrs() {
        T* tmp = ptr_.get();
        ptr_.reset();
        ptr_.reset(tmp, mock_deleter<T>);
    }

private:
    weak_ptr_factory(const weak_ptr_factory&) = delete;
    weak_ptr_factory(const weak_ptr_factory&&) = delete;
    weak_ptr_factory& operator=(const weak_ptr_factory&) = delete;
};

}  // namespace base
