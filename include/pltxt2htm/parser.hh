#pragma once

#include <memory>
#include <fast_io/fast_io_dsal/vector.h>
#include <fast_io/fast_io_dsal/string.h>
#include <fast_io/fast_io_dsal/string_view.h>
#include <exception/exception.hh>
#include "heap_guard.hh"
#include "astnode/basic.hh"
#include "astnode/html.hh"
#include "pltxt2htm/astnode/plext.hh"
#include "push_macro.hh"

namespace pltxt2htm {

namespace details {

/**
 * @return pltext[i]
 */
template<bool ndebug>
#if __has_cpp_attribute(__gnu__::always_inline)
[[__gnu__::always_inline]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
[[nodiscard]]
constexpr auto u8string_view_index(::fast_io::u8string_view pltext, ::std::size_t i) noexcept {
    pltxt2htm_assert(i < pltext.size(), "Index of parser out of bound");

    return pltext.index_unchecked(i);
}

template<bool ndebug>
#if __has_cpp_attribute(__gnu__::always_inline)
[[__gnu__::always_inline]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
[[nodiscard]]
constexpr auto u8string_view_subview(::fast_io::u8string_view pltext, ::std::size_t i) noexcept {
    if constexpr (ndebug) {
        return pltext.subview_unchecked(i);
    } else {
        return pltext.subview(i);
    }
}

/**
 * @brief Get the index-th char8_t from the string.
 * @return The char8_t at index I of str.
 */
template<::std::size_t I, char8_t first_char, char8_t... str>
    requires (I <= sizeof...(str))
[[nodiscard]]
consteval char8_t pack_indexing_char8_t() noexcept {
    // https://en.cppreference.com/w/cpp/language/pack_indexing.html
    if constexpr (I == 0) {
        return first_char;
    } else {
#if __cpp_pack_indexing >= 202311L
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wc++26-extensions"
    #endif
        return str...[I - 1];
    #if defined(__clang__)
        #pragma clang diagnostic pop
    #endif
#else
        return pack_indexing_char8_t<I - 1, str...>();
#endif
    }
}

/**
 * @brief Check whether the string is a prefix (without distinguishing
 *        between uppercase and lowercase) of the pl-text.
 *        This is a magic funciton that will generate if-expression in compile time.
 * @tparam prefix_str: The prefix string.
 * @param str: The string to be checked.
 * @return Whether the string is a prefix of the pl-text.
 */
template<bool ndebug, char8_t... prefix_str>
    requires ((prefix_str < 'A' || prefix_str > 'Z') && ...)
#if __has_cpp_attribute(__gnu__::always_inline)
[[__gnu__::always_inline]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
[[nodiscard]]
constexpr bool is_prefix_match(::fast_io::u8string_view str) noexcept {
    // TODO use template-for to refactor this function
    return [str]<::std::size_t... Is>(::std::index_sequence<Is...>) {
        // Check whether the index is out of bound.
        if (sizeof...(prefix_str) + 1 > str.size()) [[unlikely]] {
            return false;
        }
        // Check whether the string is a prefix of the pl-text.
        return ([str]<::std::size_t I>() {
            constexpr auto expect = pack_indexing_char8_t<I, prefix_str...>();
            if constexpr ('a' <= expect && expect <= 'z') {
                // ASCII between lowercase and uppercase is 32 (e.g. 'a' - 'A' == 32)
                constexpr char8_t diff{32};
                // (expect != str[I] && expect != str[I] + diff) <=> (expect != (str[I] | diff))
                if (expect != (::pltxt2htm::details::u8string_view_index<ndebug>(str, I) | diff)) {
                    return false;
                } else {
                    return true;
                }
            } else {
                if (expect != ::pltxt2htm::details::u8string_view_index<ndebug>(str, I)) {
                    return false;
                } else {
                    return true;
                }
            }
        }.template operator()<Is>() &&
                ...);
    }(::std::make_index_sequence<sizeof...(prefix_str)>{});
}

/**
 * @brief Cause `parse_pltxt` call itself recursively, I use `ExternSyntaxType`
 *        to mark whether the current call has extern surrounding html, markdown e.t.c syntax or not.
 */
enum class ExternSyntaxType : ::std::uint_least32_t {
    // No embrassing syntax outside
    none = 0,

    // Quantum-Physics <color> tag
    pl_color,
    // Quantum-Physics <a> tag
    pl_a,
    // Quantum-Physics <discussion> tag
    pl_discussion,
    // Quantum-Physics <experiment> tag
    pl_experiment,

    // markdown header
    // md_header,
};

/**
 * @brief Parse pl-text to nodes.
 * @tparam ndebug: Whether disables all debug checks.
 * @param pltext: The text readed from Quantum-Physics.
 * @param extern_syntax_type: show `enum class ExternSyntaxType`
 * @param extern_index: after parsing the current tag, extern index should be corrected.
 * @return Quantum-Physics text's ast.
 */
template<bool ndebug>
[[nodiscard]]
constexpr auto parse_pltxt(
    ::fast_io::u8string_view pltext,
    ::pltxt2htm::details::ExternSyntaxType extern_syntax_type = ::pltxt2htm::details::ExternSyntaxType::none,
    ::std::size_t* extern_index = nullptr)
#ifndef PLTXT2HTM_ENABLE_PRINT
    noexcept
#endif
    -> ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> {
    pltxt2htm_assert(
        (extern_syntax_type == ::pltxt2htm::details::ExternSyntaxType::none && extern_index == nullptr) ||
            (extern_syntax_type != ::pltxt2htm::details::ExternSyntaxType::none && extern_index != nullptr),
        "Invalid extern_syntax_type with extern_index");

    auto result = ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>{};

    ::std::size_t const pltxt_size{pltext.size()};
    for (::std::size_t i{}; i < pltxt_size; ++i) {
        char8_t const chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i)};

        if (chr == u8'\n') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Br>{});
            continue;
        } else if (chr == u8' ') {
            // TODO should we delete tail space?
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Space>{});
            continue;
        } else if (chr == u8'&') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Ampersand>{});
            continue;
        } else if (chr == u8'\'') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::SingleQuotationMark>{});
            continue;
        } else if (chr == u8'\"') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::DoubleQuotationMark>{});
            continue;
        } else if (chr == u8'>') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::GreaterThan>{});
            continue;
        } else if (chr == u8'<') {
            // if i is a valid value, i always less than pltxt_size
            pltxt2htm_assert(i < pltxt_size, "Index of parser out of bound");

            if (i + 1 == pltxt_size) {
                goto not_valid_tag;
            }

            // a trie for tags
            switch (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)) {
            case u8'a':
                [[fallthrough]];
            case u8'A': {
                // parsing pl <a></a> tag (not html <a> tag)
                if (i + 2 >= pltxt_size) {
                    goto not_valid_tag;
                }
                {
                    ::std::size_t forward_index{i + 2};
                    while (true) {
                        auto forward_chr =
                            ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index);
                        if (forward_chr == u8'>') {
                            break;
                        } else if (forward_chr != ' ') {
                            goto not_valid_tag;
                        }

                        if (forward_index + 1 < pltxt_size) {
                            ++forward_index;
                        } else {
                            goto not_valid_tag;
                        }
                    }
                    i = forward_index;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::details::ExternSyntaxType::pl_a, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Color>(
                            ::std::move(subast), ::fast_io::u8string{u8"#0000AA"}));
                    }
                    goto complete_parsing_tag;
                }
                ::exception::unreachable<ndebug>();
            }

            case u8'b':
                [[fallthrough]];
            case u8'B': {
                // parsing: <br>, <bR  >, <BR/>, <br  />
                if (::pltxt2htm::details::is_prefix_match<ndebug, u8'r'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                    goto not_valid_tag;
                }
                {
                    ::std::size_t forward_index{i + 3};
                    while (true) {
                        char8_t const forward_chr{
                            ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                        if (forward_chr == u8'>') {
                            i = forward_index;
                            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Br>{});
                            goto complete_parsing_tag;
                        } else if (forward_chr == u8'/' && forward_index + 1 < pltxt_size &&
                                   ::pltxt2htm::details::u8string_view_index<ndebug>(
                                       pltext, forward_index + 1) == u8'>') {
                            i = forward_index + 1;
                            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Br>{});
                            goto complete_parsing_tag;
                        } else if (forward_chr != u8' ') {
                            goto not_valid_tag;
                        }
                        if (forward_index + 1 < pltxt_size) {
                            ++forward_index;
                        } else {
                            goto not_valid_tag;
                        }
                    }
                }

                ::exception::unreachable<ndebug>();
            }

            case u8'c':
                [[fallthrough]];
            case u8'C': {
                // parsing: <color=$1>$2</color>
                if (::pltxt2htm::details::is_prefix_match<ndebug, u8'o', u8'l', u8'o', u8'r', u8'='>(
                        u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                    goto not_valid_tag;
                }
                {
                    ::std::size_t forward_index{i + 7};
                    ::fast_io::u8string color{};
                    while (true) {
                        char8_t const forward_chr{
                            ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                        if (forward_chr == u8'>') {
                            break;
                        } else if (forward_chr == u8' ') {
                            while (true) {
                                if (forward_index + 1 >= pltxt_size) {
                                    goto not_valid_tag;
                                }

                                if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                        pltext, forward_index + 1) == u8' ') {
                                    ++forward_index;
                                } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                               pltext, forward_index + 1) == u8'>') {
                                    ++forward_index;
                                    break;
                                } else {
                                    goto not_valid_tag;
                                }
                            }
                            break;
                        } else {
                            color.push_back(forward_chr);
                        }
                        if (forward_index + 1 >= pltxt_size) {
                            goto not_valid_tag;
                        } else {
                            ++forward_index;
                        }
                    }
                    // parsing start tag <color> successed
                    i = forward_index;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::details::ExternSyntaxType::pl_color, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Color>(::std::move(subast),
                                                                                             ::std::move(color)));
                    }
                    goto complete_parsing_tag;
                }
                ::exception::unreachable<ndebug>();
            }

            case u8'd':
                [[fallthrough]];
            case u8'D': {
                // parsing: <discussion=$1>$2</discussion>
                if (::pltxt2htm::details::is_prefix_match<ndebug, u8'i', u8's', u8'c', u8'u', u8's', u8's',
                                                          u8'i', u8'o', u8'n', u8'='>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                    goto not_valid_tag;
                }
                {
                    ::std::size_t forward_index{i + 12};
                    ::fast_io::u8string id{};
                    while (true) {
                        char8_t const forward_chr{
                            ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                        if (forward_chr == u8'>') {
                            break;
                        } else if (forward_chr == u8' ') {
                            while (true) {
                                if (forward_index + 1 >= pltxt_size) {
                                    goto not_valid_tag;
                                }

                                if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                        pltext, forward_index + 1) == u8' ') {
                                    ++forward_index;
                                } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                               pltext, forward_index + 1) == u8'>') {
                                    ++forward_index;
                                    break;
                                } else {
                                    goto not_valid_tag;
                                }
                            }
                            break;
                        } else {
                            id.push_back(forward_chr);
                        }
                        if (forward_index + 1 >= pltxt_size) {
                            goto not_valid_tag;
                        } else {
                            ++forward_index;
                        }
                    }
                    // parsing start tag <color> successed
                    i = forward_index;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::details::ExternSyntaxType::pl_discussion, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Discussion>(::std::move(subast),
                                                                                                  ::std::move(id)));
                    }
                    goto complete_parsing_tag;
                }
                ::exception::unreachable<ndebug>();
            }

            case u8'e':
                [[fallthrough]];
            case u8'E': {
                // parsing: <experiment=$1>$2</experiment>
                if (::pltxt2htm::details::is_prefix_match<ndebug, u8'x', u8'p', u8'e', u8'r', u8'i', u8'm',
                                                          u8'e', u8'n', u8't', u8'='>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                    goto not_valid_tag;
                }
                {
                    ::std::size_t forward_index{i + 12};
                    ::fast_io::u8string id{};
                    while (true) {
                        char8_t const forward_chr{
                            ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                        if (forward_chr == u8'>') {
                            break;
                        } else if (forward_chr == u8' ') {
                            while (true) {
                                if (forward_index + 1 >= pltxt_size) {
                                    goto not_valid_tag;
                                }

                                if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                        pltext, forward_index + 1) == u8' ') {
                                    ++forward_index;
                                } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                               pltext, forward_index + 1) == u8'>') {
                                    ++forward_index;
                                    break;
                                } else {
                                    goto not_valid_tag;
                                }
                            }
                            break;
                        } else {
                            id.push_back(forward_chr);
                        }
                        if (forward_index + 1 >= pltxt_size) {
                            goto not_valid_tag;
                        } else {
                            ++forward_index;
                        }
                    }
                    // parsing start tag <color> successed
                    i = forward_index;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::details::ExternSyntaxType::pl_experiment, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Experiment>(::std::move(subast),
                                                                                                  ::std::move(id)));
                    }
                    goto complete_parsing_tag;
                }
                ::exception::unreachable<ndebug>();
            }

            case u8'/': {
                switch (extern_syntax_type) {
                case ::pltxt2htm::details::ExternSyntaxType::pl_color: {
                    // parsing </color>
                    if (::pltxt2htm::details::is_prefix_match<ndebug, u8'c', u8'o', u8'l', u8'o', u8'r'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                        goto not_valid_tag;
                    }
                    {
                        ::std::size_t forward_index{i + 7};
                        while (true) {
                            char8_t const forward_chr{
                                ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                            if (forward_chr == u8'>') {
                                break;
                            } else if (forward_chr == u8' ') {
                                while (true) {
                                    if (forward_index + 1 >= pltxt_size) {
                                        goto not_valid_tag;
                                    }

                                    if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                            pltext, forward_index + 1) == u8' ') {
                                        ++forward_index;
                                    } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                                   pltext, forward_index + 1) == u8'>') {
                                        ++forward_index;
                                        break;
                                    } else {
                                        goto not_valid_tag;
                                    }
                                }
                                break;
                            } else {
                                goto not_valid_tag;
                            }
                            if (forward_index + 1 >= pltxt_size) {
                                goto not_valid_tag;
                            } else {
                                ++forward_index;
                            }
                        }
                        // parsing end tag </color> successed
                        // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                        if (extern_index != nullptr) {
                            *extern_index += forward_index + 1;
                        }
                        return result;
                    }
                    ::exception::unreachable<ndebug>();
                }
                case ::pltxt2htm::details::ExternSyntaxType::pl_a: {
                    if (::pltxt2htm::details::is_prefix_match<ndebug, u8'a'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                        goto not_valid_tag;
                    }
                    {
                        ::std::size_t forward_index{i + 3};
                        while (true) {
                            char8_t const forward_chr{
                                ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                            if (forward_chr == u8'>') {
                                break;
                            } else if (forward_chr == u8' ') {
                                while (true) {
                                    if (forward_index + 1 >= pltxt_size) {
                                        goto not_valid_tag;
                                    }

                                    if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                            pltext, forward_index + 1) == u8' ') {
                                        ++forward_index;
                                    } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                                   pltext, forward_index + 1) == u8'>') {
                                        ++forward_index;
                                        break;
                                    } else {
                                        goto not_valid_tag;
                                    }
                                }
                                break;
                            } else {
                                goto not_valid_tag;
                            }
                        }
                        // parsing end tag </a> successed
                        if (extern_index != nullptr) {
                            *extern_index += forward_index + 1;
                        }
                        return result;
                    }
                    ::exception::unreachable<ndebug>();
                }
                case ::pltxt2htm::details::ExternSyntaxType::none: {
                    goto not_valid_tag;
                }
                case ::pltxt2htm::details::ExternSyntaxType::pl_discussion: {
                    // parsing </discussion>
                    if (::pltxt2htm::details::is_prefix_match<ndebug, u8'd', u8'i', u8's', u8'c', u8'u',
                                                              u8's', u8's', u8'i', u8'o', u8'n'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                        goto not_valid_tag;
                    }
                    {
                        ::std::size_t forward_index{i + 12};
                        while (true) {
                            char8_t const forward_chr{
                                ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                            if (forward_chr == u8'>') {
                                break;
                            } else if (forward_chr == u8' ') {
                                while (true) {
                                    if (forward_index + 1 >= pltxt_size) {
                                        goto not_valid_tag;
                                    }

                                    if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                            pltext, forward_index + 1) == u8' ') {
                                        ++forward_index;
                                    } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                                   pltext, forward_index + 1) == u8'>') {
                                        ++forward_index;
                                        break;
                                    } else {
                                        goto not_valid_tag;
                                    }
                                }
                                break;
                            } else {
                                goto not_valid_tag;
                            }
                            if (forward_index + 1 >= pltxt_size) {
                                goto not_valid_tag;
                            } else {
                                ++forward_index;
                            }
                        }
                        // parsing end tag </color> successed
                        // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                        if (extern_index != nullptr) {
                            *extern_index += forward_index + 1;
                        }
                        return result;
                    }
                    ::exception::unreachable<ndebug>();
                }
                case ::pltxt2htm::details::ExternSyntaxType::pl_experiment: {
                    // parsing </experiment>
                    if (::pltxt2htm::details::is_prefix_match<ndebug, u8'e', u8'x', u8'p', u8'e', u8'r',
                                                              u8'i', u8'm', u8'e', u8'n', u8't'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2)) == false) {
                        goto not_valid_tag;
                    }
                    {
                        ::std::size_t forward_index{i + 12};
                        while (true) {
                            char8_t const forward_chr{
                                ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
                            if (forward_chr == u8'>') {
                                break;
                            } else if (forward_chr == u8' ') {
                                while (true) {
                                    if (forward_index + 1 >= pltxt_size) {
                                        goto not_valid_tag;
                                    }

                                    if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                            pltext, forward_index + 1) == u8' ') {
                                        ++forward_index;
                                    } else if (::pltxt2htm::details::u8string_view_index<ndebug>(
                                                   pltext, forward_index + 1) == u8'>') {
                                        ++forward_index;
                                        break;
                                    } else {
                                        goto not_valid_tag;
                                    }
                                }
                                break;
                            } else {
                                goto not_valid_tag;
                            }
                            if (forward_index + 1 >= pltxt_size) {
                                goto not_valid_tag;
                            } else {
                                ++forward_index;
                            }
                        }
                        // parsing end tag </color> successed
                        // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                        if (extern_index != nullptr) {
                            *extern_index += forward_index + 1;
                        }
                        return result;
                    }
                    ::exception::unreachable<ndebug>();
                }
#if 0
                default:
                    ::exception::unreachable<ndebug>();
#endif
                }
                ::exception::unreachable<ndebug>();
            }

            default:
                goto not_valid_tag;
            }

        not_valid_tag:
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
        complete_parsing_tag:
            continue;
        } else if (chr <= 0x1f || (0x7f <= chr && chr <= 0x9f)) {
            // utf-8 control characters will be ignored
            continue;
        } else {
            // normal utf-8 characters
            if ((chr & 0b1000'0000) == 0) {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
            } else if ((chr & 0b1100'0000) == 0b1100'0000 && (chr & 0b0010'0000) == 0) {
                pltxt2htm_assert(i + 1 < pltxt_size, "Invalid utf-8 encoding");

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)});
                i += 1;
            } else if ((chr & 0b1110'0000) == 0b1110'0000 && (chr & 0b0001'0000) == 0) {
                pltxt2htm_assert(i + 2 < pltxt_size, "Invalid utf-8 encoding");
                ;

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 2)});
                i += 2;
            } else if ((chr & 0b1111'0000) == 0b1111'0000 && (chr & 0b0000'1000) == 0) {
                pltxt2htm_assert(i + 3 < pltxt_size, "Invalid utf-8 encoding");

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 2)});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 3)});
                i += 3;
            } else [[unlikely]] {
                // invalid utf-8 encoding
                ::exception::unreachable<ndebug>();
            }
        }
    }

    if (extern_index != nullptr) {
        *extern_index += pltxt_size;
    }
    return result;
}

} // namespace details

/**
 * @brief Impl of parse pl-text to nodes.
 * @tparam ndebug: Whether or not to disable debugging checks (like NDEBUG macro).
 * @tparam disable_log: Whether or not to disable print info before crashing.
 * @param pltext: The text readed from Quantum-Physics.
 */
template<bool ndebug>
[[nodiscard]]
constexpr auto parse_pltxt(::fast_io::u8string_view pltext)
#ifndef PLTXT2HTM_ENABLE_PRINT
    noexcept
#endif
{
    return ::pltxt2htm::details::parse_pltxt<ndebug>(pltext);
}

} // namespace pltxt2htm

#include "pop_macro.hh"
