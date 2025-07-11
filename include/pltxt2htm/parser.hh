#pragma once

#include <memory>
#include <fast_io/fast_io_dsal/vector.h>
#include <fast_io/fast_io_dsal/string.h>
#include <fast_io/fast_io_dsal/string_view.h>
#include <exception/exception.hh>
#include "astnode/node_type.hh"
#include "heap_guard.hh"
#include "astnode/basic.hh"
#include "astnode/html_node.hh"
#include "astnode/physics_lab_node.hh"
#include "push_macro.hh"

namespace pltxt2htm {

namespace details {

/**
 * @return index of ::fast_io::u8string_view
 */
template<bool ndebug>
#if __has_cpp_attribute(__gnu__::always_inline)
[[__gnu__::always_inline]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
[[nodiscard]]
constexpr auto u8string_view_index(::fast_io::u8string_view pltext, ::std::size_t i)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    pltxt2htm_assert(i < pltext.size(), u8"Index of parser out of bound");

    return pltext.index_unchecked(i);
}

/**
 * @brief Get the slice of u8string_view
 */
template<bool ndebug>
#if __has_cpp_attribute(__gnu__::always_inline)
[[__gnu__::always_inline]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
[[nodiscard]]
constexpr auto u8string_view_subview(::fast_io::u8string_view pltext, ::std::size_t i,
                                     ::std::size_t count = ::fast_io::containers::npos) noexcept
    -> ::fast_io::u8string_view {
    if constexpr (ndebug) {
        return pltext.subview_unchecked(i, count);
    } else {
        return pltext.subview(i, count);
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
constexpr bool is_prefix_match(::fast_io::u8string_view str)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    // TODO use template-for to refactor this function
    return [str]<::std::size_t... Is>(::std::index_sequence<Is...>) {
        // Check whether the index is out of bound.
        if (sizeof...(prefix_str) >= str.size()) [[unlikely]] {
            return false;
        }
        // Return whether the `prefix_str` is a prefix of the str.
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
 * @brief same as regex /^tag_name *\>/
 * @example parsing <i>, <b>, <h3> e.t.c
 * @tparam prefix_str: The prefix string.
 * @param str: The string to be checked.
 * @return Whether the string is a prefix of the pl-text.
 */
template<bool ndebug, char8_t... tag_name>
[[nodiscard]]
constexpr bool try_parse_bare_tag(::fast_io::u8string_view pltext, ::std::size_t& extern_index)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    if (::pltxt2htm::details::is_prefix_match<ndebug, tag_name...>(pltext) == false) {
        return false;
    }

    auto const pltxt_size = pltext.size();
    for (::std::size_t i{sizeof...(tag_name)}; i < pltxt_size; ++i) {
        auto forward_chr = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i);
        if (forward_chr == u8'>') {
            extern_index = i;
            return true;
        } else if (forward_chr != ' ') {
            return false;
        }
    }
    return false;
}

/**
 * @brief parsing `Tag=$1>`
 * @param[out] substr: str of $1
 */
template<bool ndebug, char8_t... prefix_str>
[[nodiscard]]
constexpr bool try_parse_equal_sign_tag(::fast_io::u8string_view pltext, ::std::size_t& extern_index,
                                        ::fast_io::u8string& substr)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    if (::pltxt2htm::details::is_prefix_match<ndebug, prefix_str..., u8'='>(pltext) == false) {
        return false;
    }

    auto const pltxt_size = pltext.size();
    ::std::size_t forward_index{sizeof...(prefix_str) + 1};
    while (true) {
        char8_t const forward_chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
        if (forward_chr == u8'>') {
            extern_index = forward_index;
            return true;
        } else if (forward_chr == u8' ') {
            while (true) {
                if (forward_index + 1 >= pltxt_size) {
                    return false;
                }

                if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index + 1) == u8' ') {
                    ++forward_index;
                } else if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index + 1) == u8'>') {
                    extern_index = forward_index + 1;
                    return true;
                } else {
                    return false;
                }
            }
        } else {
            substr.push_back(forward_chr);
        }
        if (forward_index + 1 >= pltxt_size) {
            return false;
        } else {
            ++forward_index;
        }
    }
}

/**
 * @brief try to parsing `<tag_name>` or `<tag_name/>`
 * @param[in] pltext: source text
 * @param[out] i: length of tag
 * @return 0: fail; Otherwise: success
 */
template<bool ndebug, char8_t... tag_name>
[[nodiscard]]
constexpr bool try_parse_self_closing_tag(::fast_io::u8string_view pltext, ::std::size_t& extern_index) noexcept {
    ::std::size_t const pltxt_size{pltext.size()};
    if (::pltxt2htm::details::is_prefix_match<ndebug, tag_name...>(pltext) == false) {
        return false;
    }

    for (::std::size_t forward_index{sizeof...(tag_name)}; forward_index < pltxt_size; ++forward_index) {
        char8_t const forward_chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
        if (forward_chr == u8'>') {
            extern_index = forward_index;
            return true;
        } else if (forward_chr == u8'/' && forward_index + 1 < pltxt_size &&
                   ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index + 1) == u8'>') {
            extern_index = forward_index + 1;
            return true;
        } else if (forward_chr != u8' ') {
            return false;
        }
    }
    return false;
}

/**
 * @brief Parsing markdown ATX headings
 * TODO more notes
 */
template<bool ndebug>
[[nodiscard]]
constexpr bool try_parse_md_atx_heading(::fast_io::u8string_view pltext, ::std::size_t& start_index_,
                                        ::std::size_t& end_index_, ::std::size_t& header_level_)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    ::std::size_t const pltext_size{pltext.size()};
    ::std::size_t start_index{};
    while (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) == u8' ') {
        if (start_index >= pltext_size) {
            return false;
        }
        ++start_index;
    }

    // count how many `#` characters
    ::std::size_t header_level{};
    while (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) == u8'#') {
        if (start_index >= pltext_size) {
            return false;
        }
        ++start_index;
        ++header_level;
    }
    if (header_level == 0 || header_level > 6 ||
        ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) != u8' ') {
        // invalid atx header
        return false;
    }
    ++start_index;
    // end of the atx header
    ::std::size_t end_index{start_index};
    while (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, end_index) != u8'\n' &&
           !::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'r'>(pltext, end_index)) {
        if (end_index >= pltext_size) {
            break;
        }
        ++end_index;
    }
    start_index_ = start_index;
    end_index_ = end_index;
    header_level_ = header_level;
    return true;
}

/**
 * @brief Convert a string to a ::std::size_t.
 */
[[nodiscard]]
constexpr ::exception::optional<std::size_t> u8str2size_t(::fast_io::u8string_view str) noexcept {
    if (str.empty()) {
        return ::exception::nullopt_t{};
    }

    ::std::size_t result{};
    for (auto&& c : str) {
        if (c < u8'0' || c > u8'9') {
            return ::exception::nullopt_t{};
        }

        result = result * 10 + (c - '0');
    }

    return result;
}

/**
 * @brief Parse pl-text to nodes.
 * @tparam ndebug: Whether disables all debug checks.
 * @param pltext: The text readed from Quantum-Physics.
 * @param extern_syntax_type: if extern_syntax_type is ::pltxt2htm::NodeType::base, means
 *                            no nested nodes.
 * @param extern_index: after parsing the current tag, extern index should be corrected.
 * @return Quantum-Physics text's ast.
 */
template<bool ndebug>
[[nodiscard]]
constexpr auto parse_pltxt(::fast_io::u8string_view pltext,
                           ::pltxt2htm::NodeType extern_syntax_type = ::pltxt2htm::NodeType::base,
                           ::std::size_t* extern_index = nullptr)
#if __cpp_exceptions < 199711L
    noexcept
#endif
    -> ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> {
    pltxt2htm_assert((extern_syntax_type == ::pltxt2htm::NodeType::base && extern_index == nullptr) ||
                         (extern_syntax_type != ::pltxt2htm::NodeType::base && extern_index != nullptr),
                     u8"Invalid extern_syntax_type with extern_index");

    auto result = ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>{};

    ::std::size_t const pltxt_size{pltext.size()};
    for (::std::size_t i{}; i < pltxt_size; ++i) {
        char8_t const chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i)};

        if (chr == u8'\n') {
#if __has_cpp_attribute(indeterminate)
            ::std::size_t start_index [[indeterminate]];
            ::std::size_t end_index [[indeterminate]];
            ::std::size_t header_level [[indeterminate]];
#else
            ::std::size_t start_index;
            ::std::size_t end_index;
            ::std::size_t header_level;
#endif
            if (::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(
                    ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1), start_index, end_index,
                    header_level)) {
                //
            } else {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LineBreak>{});
            }
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
            pltxt2htm_assert(i < pltxt_size, u8"Index of parser out of bound");

            if (i + 1 == pltxt_size) {
                goto not_valid_tag;
            }

            // a trie for tags
            switch (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)) {
            case u8'a':
                [[fallthrough]];
            case u8'A': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                // parsing pl <a>$1</a> tag (not html <a> tag)
                if (::pltxt2htm::details::try_parse_bare_tag<ndebug>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_a, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::A>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'b':
                [[fallthrough]];
            case u8'B': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                if (::pltxt2htm::details::try_parse_bare_tag<ndebug>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing pl&html <b> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_b, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::B>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'r'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    i += tag_len + 2;
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Br>(::pltxt2htm::Br()));
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'c':
                [[fallthrough]];
            case u8'C': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                // parsing: <color=$1>$2</color>
                ::fast_io::u8string color;
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8'o', u8'l', u8'o', u8'r'>(
                        u8string_view_subview<ndebug>(pltext, i + 2), tag_len, color)) {
                    i += tag_len + 2;
                    // parsing start tag <color> successed
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_color, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Color>(::std::move(subast),
                                                                                             ::std::move(color)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'd':
                [[fallthrough]];
            case u8'D': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'e', u8'l'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing <del>$1</del>
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_del, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Del>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                }
                // parsing: <discussion=$1>$2</discussion>
                ::fast_io::u8string id{};
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8'i', u8's', u8'c', u8'u', u8's', u8's',
                                                                   u8'i', u8'o', u8'n'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len, id)) {
                    i += tag_len + 2;
                } else {
                    goto not_valid_tag;
                }

                if (i + 1 < pltxt_size) {
                    // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                    // which does not make sense, can be opetimized(ignored) during parsing ast
                    auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                        ::pltxt2htm::NodeType::pl_discussion, ::std::addressof(i));
                    result.push_back(
                        ::pltxt2htm::details::HeapGuard<::pltxt2htm::Discussion>(::std::move(subast), ::std::move(id)));
                }
                goto complete_parsing_tag;
            }

            case u8'e':
                [[fallthrough]];
            case u8'E': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                // parsing: <experiment=$1>$2</experiment>
                ::fast_io::u8string id{};
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8'x', u8'p', u8'e', u8'r', u8'i', u8'm',
                                                                   u8'e', u8'n', u8't'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len, id)) {
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_experiment, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Experiment>(::std::move(subast),
                                                                                                  ::std::move(id)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'h':
                [[fallthrough]];
            case u8'H': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'1'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing html <h1> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_h1, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H1>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'2'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing html <h2> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_h2, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H2>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'3'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing html <h3> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_h3, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H3>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'4'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing html <h4> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_h4, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H4>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'5'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing html <h5> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_h5, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H5>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'6'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    // parsing html <h6> tag
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_h6, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H6>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'i':
                [[fallthrough]];
            case u8'I': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                // parsing pl&html <i>$1</i> tag
                if (::pltxt2htm::details::try_parse_bare_tag<ndebug>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_i, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::I>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'p':
                [[fallthrough]];
            case u8'P': {
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                // parsing html <p></p> tag
                if (::pltxt2htm::details::try_parse_bare_tag<ndebug>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len)) {
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::html_p, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::P>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8's':
                [[fallthrough]];
            case u8'S': {
// parsing pl <size=$1>$2</size> tag
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                ::fast_io::u8string id_{};
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8'i', u8'z', u8'e'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len, id_)) {
                    i += tag_len + 2;
                    auto id{::pltxt2htm::details::u8str2size_t(::fast_io::mnp::os_c_str(id_))};
                    if (!id.has_value()) {
                        goto not_valid_tag;
                    }

                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_size, ::std::addressof(i));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Size>(
                            ::std::move(subast), ::std::move(id.template value<ndebug>())));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'u':
                [[fallthrough]];
            case u8'U': {
// parsing pl <user=$1>$2</user> tag
#if __has_cpp_attribute(indeterminate)
                ::std::size_t tag_len [[indeterminate]];
#else
                ::std::size_t tag_len;
#endif
                ::fast_io::u8string id{};
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8's', u8'e', u8'r'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len, id)) {
                    i += tag_len + 2;
                    if (i + 1 < pltxt_size) {
                        // if forward_index + 1 >= pltxt_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 1),
                            ::pltxt2htm::NodeType::pl_user, ::std::addressof(i));
                        result.push_back(
                            ::pltxt2htm::details::HeapGuard<::pltxt2htm::User>(::std::move(subast), ::std::move(id)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'/': {
                switch (extern_syntax_type) {
                case ::pltxt2htm::NodeType::pl_color: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'c', u8'o', u8'l', u8'o', u8'r'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </color> successed
                    // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_a: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    // parsing </a>
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'a'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </a> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_experiment: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    // parsing </experiment>
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'e', u8'x', u8'p', u8'e', u8'r', u8'i',
                                                                 u8'm', u8'e', u8'n', u8't'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_discussion: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    // parsing </discussion>
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'd', u8'i', u8's', u8'c', u8'u', u8's',
                                                                 u8's', u8'i', u8'o', u8'n'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_user: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    // parsing </user>
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'u', u8's', u8'e', u8'r'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_size: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    // parsing </size>
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8's', u8'i', u8'z', u8'e'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_b: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'b'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </b> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::pl_i: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'i'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </a> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_p: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'p'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </p> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_h1: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'h', u8'1'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h1> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_h2: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'h', u8'2'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h2> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_h3: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'h', u8'3'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h3> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_h4: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'h', u8'4'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h4> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_h5: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'h', u8'5'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h5> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_h6: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'h', u8'6'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h6> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                case ::pltxt2htm::NodeType::html_del: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'd', u8'e', u8'l'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, i + 2), tag_len) == false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </del> successed
                    if (extern_index != nullptr) {
                        *extern_index += i + tag_len + 3;
                    }
                    return result;
                }
                default:
                    goto not_valid_tag;
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
                pltxt2htm_assert(i + 1 < pltxt_size, u8"Invalid utf-8 encoding");

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)});
                i += 1;
            } else if ((chr & 0b1110'0000) == 0b1110'0000 && (chr & 0b0001'0000) == 0) {
                pltxt2htm_assert(i + 2 < pltxt_size, u8"Invalid utf-8 encoding");

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 1)});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{
                    ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, i + 2)});
                i += 2;
            } else if ((chr & 0b1111'0000) == 0b1111'0000 && (chr & 0b0000'1000) == 0) {
                pltxt2htm_assert(i + 3 < pltxt_size, u8"Invalid utf-8 encoding");

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
 * @param pltext: The text readed from Quantum-Physics.
 */
template<bool ndebug>
[[nodiscard]]
constexpr auto parse_pltxt(::fast_io::u8string_view pltext)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    return ::pltxt2htm::details::parse_pltxt<ndebug>(pltext);
}

} // namespace pltxt2htm

#include "pop_macro.hh"
