#pragma once

/**
 * @file pltxt2htm.h
 * @brief This file provides c pointer style interface wrapped ::pltxt2htm::pltxt2html
 */

#include <cstdlib>
#include <cstring>
#include <concepts>
#include <exception/exception.hh>
#include "backend.hh"
#include "pltxt2htm.hh"

namespace pltxt2htm {

namespace details {

template<auto Func>
[[nodiscard]]
constexpr char8_t const* c_ptr_style_wrapper(char8_t const* const text, char8_t const* const host) noexcept(
    noexcept((Func(::fast_io::mnp::os_c_str(text), ::fast_io::mnp::os_c_str(host))))) {
    auto html = Func(::fast_io::mnp::os_c_str(text), ::fast_io::mnp::os_c_str(host));
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
 * @brief C-Pointer-Style interface for C++ API pltxt2htm::pltxt2html <::pltxt2htm::BackendText::advanced_html>
 * @note Don't forget to free the returned pointer
 */
template<bool ndebug = false>
[[nodiscard]]
constexpr char8_t const* advanced_parser(char8_t const* const text,
                                         char8_t const* const host)
#ifndef PLTXT2HTM_ENABLE_PRINT
    noexcept
#endif
{
    return ::pltxt2htm::details::c_ptr_style_wrapper<
        ::pltxt2htm::pltxt2html<::pltxt2htm::BackendText::advanced_html, ndebug>>(text, host);
}

/**
 * @brief C-Pointer-Style interface for C++ API pltxt2htm::pltxt2html <::pltxt2htm::BackendText::advanced_html>
 * @note Don't forget to free the returned pointer
 */
template<bool ndebug = false, bool disable_log = true>
[[nodiscard]]
constexpr char8_t const* common_parser(char8_t const* const text,
                                       char8_t const* const host)
#ifndef PLTXT2HTM_ENABLE_PRINT
    noexcept
#endif
{
    return ::pltxt2htm::details::c_ptr_style_wrapper<
        ::pltxt2htm::pltxt2html<::pltxt2htm::BackendText::common_html, ndebug>>(text, host);
}

} // namespace pltxt2htm
