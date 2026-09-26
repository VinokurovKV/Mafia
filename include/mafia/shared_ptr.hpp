#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

namespace mafia {

template <typename T>
class SharedPtr {
public:
    constexpr SharedPtr() noexcept = default;
    constexpr SharedPtr(std::nullptr_t) noexcept {}

    explicit SharedPtr(T* pointer)
        : control_(makeControl(pointer)) {}

    SharedPtr(const SharedPtr& other) noexcept
        : control_(other.control_) {
        retain();
    }

    SharedPtr(SharedPtr&& other) noexcept
        : control_(std::exchange(other.control_, nullptr)) {}

    ~SharedPtr() {
        release();
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this != &other) {
            SharedPtr copy(other);
            swap(copy);
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            SharedPtr moved(std::move(other));
            swap(moved);
        }
        return *this;
    }

    T& operator*() const noexcept {
        return *get();
    }

    T* operator->() const noexcept {
        return get();
    }

    T* get() const noexcept {
        return control_ == nullptr ? nullptr : control_->pointer;
    }

    explicit operator bool() const noexcept {
        return get() != nullptr;
    }

    void reset(T* pointer = nullptr) {
        SharedPtr replacement(pointer);
        swap(replacement);
    }

    void swap(SharedPtr& other) noexcept {
        std::swap(control_, other.control_);
    }

    std::size_t useCount() const noexcept {
        return control_ == nullptr
            ? 0
            : control_->references.load(std::memory_order_relaxed);
    }

private:
    struct ControlBlock {
        explicit ControlBlock(T* value) noexcept
            : pointer(value) {}

        std::atomic_size_t references{1};
        T* pointer;
    };

    static ControlBlock* makeControl(T* pointer) {
        if (pointer == nullptr) {
            return nullptr;
        }

        try {
            return new ControlBlock(pointer);
        } catch (...) {
            delete pointer;
            throw;
        }
    }

    void retain() noexcept {
        if (control_ != nullptr) {
            control_->references.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void release() noexcept {
        if (control_ == nullptr) {
            return;
        }

        if (control_->references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete control_->pointer;
            delete control_;
        }
        control_ = nullptr;
    }

    ControlBlock* control_ = nullptr;
};

template <typename T>
void swap(SharedPtr<T>& left, SharedPtr<T>& right) noexcept {
    left.swap(right);
}

template <typename T>
bool operator==(const SharedPtr<T>& left, const SharedPtr<T>& right) noexcept {
    return left.get() == right.get();
}

template <typename T>
bool operator!=(const SharedPtr<T>& left, const SharedPtr<T>& right) noexcept {
    return !(left == right);
}

template <typename T>
bool operator==(const SharedPtr<T>& pointer, std::nullptr_t) noexcept {
    return pointer.get() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const SharedPtr<T>& pointer) noexcept {
    return pointer == nullptr;
}

template <typename T>
bool operator!=(const SharedPtr<T>& pointer, std::nullptr_t) noexcept {
    return !(pointer == nullptr);
}

template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T>& pointer) noexcept {
    return !(pointer == nullptr);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace mafia
