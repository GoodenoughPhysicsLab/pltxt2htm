#pragma once

#include <cstdlib>
#include <memory>
#include <utility>
#include <concepts>
#include <type_traits>
#include <exception/exception.hh>

namespace pltxt2htm::details {

template<typename T>
class HeapGuard;

/**
 * @brief This internal var is a part of `concept is_heap_gurad`
 */
template<typename T>
constexpr bool is_heap_guard_ = false;

/**
 * @brief This internal var is a part of `concept is_heap_gurad`
 */
template<typename T>
constexpr bool is_heap_guard_<HeapGuard<T>> = true;

/**
 * @brief Whether the type is a heap guard
 * @tparam type to check
 */
template<typename T>
concept is_heap_guard = is_heap_guard_<::std::remove_cvref_t<T>>;

/**
 * @brief RAII a heap allocated pointer
 * @tparam type to allocate
 */
template<typename T>
class HeapGuard {
    T* ptr_;

public:
    using value_type = T;

    template<typename... Args>
        requires (((!::pltxt2htm::details::is_heap_guard<Args>) && ...) && ::std::constructible_from<T, Args...>)
    constexpr HeapGuard(Args&&... args) noexcept {
        this->ptr_ = reinterpret_cast<T*>(::std::malloc(sizeof(T)));
        if (this->ptr_ == nullptr) [[unlikely]] {
            // bad alloc should never be an exception or err_code
            ::exception::terminate();
        }
        ::std::construct_at(this->ptr_, ::std::forward<Args>(args)...);
    }

    constexpr HeapGuard(HeapGuard<T> const& other) noexcept
        requires (::std::is_copy_constructible_v<T>)
    {
        // ::std::malloc will implicitly start lifetime of ptr
        // Therefore, should not call ::std::start_lifetime_as
        this->ptr_ = reinterpret_cast<T*>(::std::malloc(sizeof(T)));
        if (this->ptr_ == nullptr) [[unlikely]] {
            // bad alloc should never be an exception or err_code
            ::exception::terminate();
        }
        ::std::construct_at(this->ptr_, *other.release_imul());
    }

    template<typename U>
        requires (::std::derived_from<U, T>)
    constexpr HeapGuard(HeapGuard<U>&& other) noexcept
        requires (::std::is_move_constructible_v<T>)
    {
        this->ptr_ = other.release();
    }

    constexpr HeapGuard& operator=(HeapGuard<T> const&) noexcept = delete
#if __cpp_deleted_function >= 202403L
        ("Despite copy a HeapGuard is safe, but the HeapGuard's behavior is more similar to std::unique_ptr")
#endif
        ;

    template<bool ndebug = false>
    constexpr HeapGuard& operator=([[maybe_unused]] this HeapGuard<T>& self,
                                   [[maybe_unused]] HeapGuard<T>&& other) noexcept {
#if 1
        static_assert(false, "This move assignment is useless in pltxt2htm");
#else
        pltxt2htm_assert(self.ptr_ != other.release_imul(), u8"Assigning to self is not allowed");
        self.swap(self, other);
        return self;
#endif
    }

    constexpr ~HeapGuard() noexcept {
        if (ptr_ != nullptr) {
            ::std::destroy_at(this->ptr_);
            ::std::free(this->ptr_);
        }
    }

    constexpr T* operator->(this ::pltxt2htm::details::HeapGuard<T>& self) noexcept {
        return self.ptr_;
    }

    constexpr T const* operator->(this ::pltxt2htm::details::HeapGuard<T> const& self) noexcept {
        return self.ptr_;
    }

    [[nodiscard]]
    constexpr T const* release_imul(this ::pltxt2htm::details::HeapGuard<T> const& self) noexcept {
        return self.ptr_;
    }

    /**
     * @note The result is a borrowed reference.
     */
    [[nodiscard]]
    constexpr T* release_unsafe(this auto&& self) noexcept {
        return self.ptr_;
    }

    [[nodiscard]]
    constexpr T* release(this auto&& self) noexcept {
        T* ptr = self.ptr_;
        self.ptr_ = nullptr;
        return ptr;
    }

    constexpr void swap(this HeapGuard<T>& self, HeapGuard<T>& other) noexcept {
        T* tmp = self.ptr_;
        self.ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }
};

} // namespace pltxt2htm::details
