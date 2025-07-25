#pragma once

/**
 * @file pltxt2htm.h
 * @brief This file provides c pointer style interface wrapped ::pltxt2htm::pltxt2html
 */

#include <cstdlib>
#include <cstring>
#include <utility>
#include <concepts>
#include <exception/exception.hh>
#include "pltxt2htm.hh"

namespace pltxt2htm {

namespace details {

template<auto Func, typename... Args>
[[nodiscard]]
constexpr char8_t const* c_ptr_style_wrapper(Args&&... args) noexcept(
    noexcept((Func(::std::forward<Args&&>(args)...)))) {
    auto html = Func(::std::forward<Args&&>(args)...);
    char8_t* result = reinterpret_cast<char8_t*>(::std::malloc(html.size() + 1));
    if (result == nullptr) [[unlikely]] {
        // bad alloc error should never be an exception or err-code
        ::exception::terminate();
    }
    ::std::memcpy(result, html.data(), html.size());
    result[html.size()] = u8'\0';
    return result;
}

} // namespace details

/**
 * @brief C-Pointer-Style interface for C++ API pltxt2htm::pltxt2advanced_html
 * @note Don't forget to free the returned pointer
 */
template<bool ndebug = false>
[[nodiscard]]
constexpr char8_t const* advanced_parser(char8_t const* const text, char8_t const* const host)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    return ::pltxt2htm::details::c_ptr_style_wrapper<::pltxt2htm::pltxt2advanced_html<ndebug>>(
        ::fast_io::mnp::os_c_str(text), ::fast_io::mnp::os_c_str(host));
}

/**
 * @brief C-Pointer-Style interface for C++ API pltxt2htm::pltxt2common_html
 * @note Don't forget to free the returned pointer
 */
template<bool ndebug = false>
[[nodiscard]]
constexpr char8_t const* common_parser(char8_t const* const text)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    return ::pltxt2htm::details::c_ptr_style_wrapper<::pltxt2htm::pltxt2common_html<ndebug>>(
        ::fast_io::mnp::os_c_str(text));
}

} // namespace pltxt2htm
