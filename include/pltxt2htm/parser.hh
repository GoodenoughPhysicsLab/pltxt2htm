#pragma once

#include <memory>
#include <fast_io/fast_io_dsal/stack.h>
#include <fast_io/fast_io_dsal/vector.h>
#include <fast_io/fast_io_dsal/string.h>
#include <fast_io/fast_io_dsal/string_view.h>
#include <exception/exception.hh>
#include "astnode/node_type.hh"
#include "heap_guard.hh"
#include "astnode/basic.hh"
#include "astnode/html_node.hh"
#include "astnode/markdown.hh"
#include "astnode/physics_lab_node.hh"
#include "pltxt2htm/heap_guard.hh"
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

    for (::std::size_t i{sizeof...(tag_name)}; i < pltext.size(); ++i) {
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

    for (::std::size_t forward_index{sizeof...(prefix_str) + 1}; forward_index < pltext.size(); ++forward_index) {
        char8_t const forward_chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
        if (forward_chr == u8'>') {
            extern_index = forward_index;
            return true;
        } else if (forward_chr == u8' ') {
            while (true) {
                if (forward_index + 1 >= pltext.size()) {
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
    }
    return false;
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
    if (::pltxt2htm::details::is_prefix_match<ndebug, tag_name...>(pltext) == false) {
        return false;
    }

    for (::std::size_t forward_index{sizeof...(tag_name)}; forward_index < pltext.size(); ++forward_index) {
        char8_t const forward_chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, forward_index)};
        if (forward_chr == u8'>') {
            extern_index = forward_index;
            return true;
        } else if (forward_chr == u8'/' && forward_index + 1 < pltext.size() &&
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
 * @param[in] pltext: input string
 * @param[out] start_index_: start index
 * @param[out] sublength: length of the heading context
 * @param[out] header_level: 1~6 => <h1> ~ <h6>
 * @return true if the parsing is successful
 */
template<bool ndebug>
[[nodiscard]]
constexpr bool try_parse_md_atx_heading(::fast_io::u8string_view pltext, ::std::size_t& start_index_,
                                        ::std::size_t& sublength_, ::std::size_t& header_level_)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
    ::std::size_t const pltext_size{pltext.size()};
    ::std::size_t start_index{};
    // spaces before the first #
    while (true) {
        if (start_index >= pltext_size) {
            return false;
        }
        if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) != u8' ') {
            break;
        }
        ++start_index;
    }

    // count how many `#` characters
    ::std::size_t header_level{};
    while (true) {
        if (start_index >= pltext_size) {
            // https://spec.commonmark.org/0.31.2/#example-79
            if (0 < header_level && header_level <= 6) {
                start_index_ = start_index;
                sublength_ = 0;
                header_level_ = header_level;
                return true;
            } else {
                return false;
            }
        }
        if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) != u8'#') {
            break;
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
    // spaces after the last #
    while (true) {
        if (start_index >= pltext_size) {
            // https://spec.commonmark.org/0.31.2/#example-79
            start_index_ = start_index;
            sublength_ = 0;
            header_level_ = header_level;
            return true;
        }
        if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) != u8' ') {
            break;
        }
        ++start_index;
    }
    // end of the atx header
    ::std::size_t sublength{start_index};
    for (::std::size_t _; sublength < pltext_size; ++sublength) {
        if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, sublength) == u8'\n' ||
            ::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'<', u8'b', u8'r'>(
                ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, sublength), _)) {
            break;
        }
    }
    start_index_ = start_index;
    sublength_ = sublength - start_index;
    header_level_ = header_level;
    return true;
}

/**
 * @brief Switch to a markdown atx header.
 * @param[in] header_level: The header level.
 * @param[out] subast: The sub ast.
 * @return True if successful.
 */
template<bool ndebug>
[[nodiscard]]
constexpr auto switch_md_atx_header(
    ::std::size_t header_level,
    ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
    -> ::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode> {
    switch (header_level) {
    case 1: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH1>(::std::move(subast));
    }
    case 2: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH2>(::std::move(subast));
    }
    case 3: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH3>(::std::move(subast));
    }
    case 4: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH4>(::std::move(subast));
    }
    case 5: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH5>(::std::move(subast));
    }
    case 6: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH6>(::std::move(subast));
    }
    default:
        ::exception::unreachable<ndebug>();
    }
}

constexpr ::exception::optional<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> switch_escape_char(
    char8_t u8char) noexcept {
    switch (u8char) {
    case u8'\\': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeBackslash>{});
    }
    case u8'!': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeExclamation>{});
    }
    case u8'\"': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeDoubleQuote>{});
    }
    case u8'#': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeHash>{});
    }
    case u8'$': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeDollar>{});
    }
    case u8'%': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapePercent>{});
    }
    case u8'&': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeAmpersand>{});
    }
    case u8'\'': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeSingleQuote>{});
    }
    case u8'(': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeLeftParen>{});
    }
    case u8')': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeRightParen>{});
    }
    case u8'*': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeAsterisk>{});
    }
    case u8'+': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapePlus>{});
    }
    case u8',': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeComma>{});
    }
    case u8'-': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeHyphen>{});
    }
    case u8'.': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeDot>{});
    }
    case u8'/': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeSlash>{});
    }
    case u8':': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeColon>{});
    }
    case u8';': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeSemicolon>{});
    }
    case u8'<': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeLessThan>{});
    }
    case u8'=': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeEquals>{});
    }
    case u8'>': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeGreaterThan>{});
    }
    case u8'?': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeQuestion>{});
    }
    case u8'@': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeAt>{});
    }
    case u8'[': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeLeftBracket>{});
    }
    case u8']': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeRightBracket>{});
    }
    case u8'^': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeCaret>{});
    }
    case u8'_': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeUnderscore>{});
    }
    case u8'`': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeBacktick>{});
    }
    case u8'{': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeLeftBrace>{});
    }
    case u8'|': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapePipe>{});
    }
    case u8'}': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeRightBrace>{});
    }
    case u8'~': {
        return static_cast<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>(
            ::pltxt2htm::details::HeapGuard<::pltxt2htm::EscapeTilde>{});
    }
    default:
        return ::exception::nullopt_t{};
    }
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
 * TODO
 * calling convention: the caller clean up the latest frame like cdecl
 */
struct ParsePltxtFrameContext {
    ::fast_io::u8string_view pltext;
    ::pltxt2htm::NodeType nested_tag_type;
    ::std::size_t current_index{};
    ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
    bool return_from_details{false};
};

struct EqualSignTagContext {
    ::fast_io::u8string_view pltext;
    ::pltxt2htm::NodeType nested_tag_type;
    ::std::size_t current_index{};
    ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
    bool return_from_details{false};
    ::fast_io::u8string id;
};

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
constexpr void parse_pltxt(::fast_io::stack<::pltxt2htm::details::ParsePltxtFrameContext>& call_stack)
#if __cpp_exceptions < 199711L
    noexcept
#endif
{
restart:
    ::std::size_t const pltext_size{call_stack.top().pltext.size()};
    auto&& pltext = call_stack.top().pltext;
    auto&& current_index = call_stack.top().current_index;
    auto&& result = call_stack.top().subast;
    for (; current_index < pltext_size; ++current_index) {
        char8_t const chr{::pltxt2htm::details::u8string_view_index<ndebug>(call_stack.top().pltext, current_index)};

        if (chr == u8'\n') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LineBreak>{});

#if __has_cpp_attribute(indeterminate)
            ::std::size_t start_index [[indeterminate]];
            ::std::size_t end_index [[indeterminate]];
            ::std::size_t header_level [[indeterminate]];
#else
            ::std::size_t start_index;
            ::std::size_t sublength;
            ::std::size_t header_level;
#endif
            // try parsing markdown atx header
            if (current_index + 1 < pltext_size &&
                ::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(
                    ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1), start_index,
                    sublength, header_level)) {
                ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
                if (current_index + start_index + 1 < pltext_size) {
                    auto subtext = ::pltxt2htm::details::u8string_view_subview<ndebug>(
                        pltext, current_index + start_index + 1, sublength);
                    call_stack.push({
                        .pltext{subtext},
                        .nested_tag_type{::pltxt2htm::NodeType::md_atx_h1},
                    });
                    goto restart;
                }
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
        } else if (chr == u8'\\') {
            if (current_index + 1 == pltext_size) {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{u8'\\'});
                continue;
            }
            auto escape_node = ::pltxt2htm::details::switch_escape_char(
                ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1));
            if (escape_node.has_value()) {
                result.push_back(::std::move(escape_node.value()));
                ++current_index;
            } else {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{u8'\\'});
            }
            continue;
        } else if (chr == u8'<') {
            // if i is a valid value, i always less than pltext_size
            pltxt2htm_assert(current_index < pltext_size, u8"Index of parser out of bound");

            if (current_index + 1 == pltext_size) {
                goto not_valid_tag;
            }

            // a trie for tags
            switch (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1)) {
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len)) {
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        call_stack.push({
                            .pltext{::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1)},
                            .nested_tag_type{::pltxt2htm::NodeType::pl_a},
                        });
                        goto restart;
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len)) {
                    // parsing pl&html <b> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        call_stack.push({
                            .pltext{::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1)},
                            .nested_tag_type{::pltxt2htm::NodeType::pl_b},
                        });
                        goto restart;
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'r'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    current_index += tag_len + 2;
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Br>(::pltxt2htm::Br()));

#if __has_cpp_attribute(indeterminate)
                    ::std::size_t start_index [[indeterminate]];
                    ::std::size_t end_index [[indeterminate]];
                    ::std::size_t header_level [[indeterminate]];
#else
                    ::std::size_t start_index;
                    ::std::size_t sublength;
                    ::std::size_t header_level;
#endif
                    // try parsing markdown atx header
                    if (current_index + 1 < pltext_size &&
                        ::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1), start_index,
                            sublength, header_level)) {
                        ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
                        if (current_index + start_index + 1 < pltext_size) {
                            auto subtext = ::pltxt2htm::details::u8string_view_subview<ndebug>(
                                pltext, current_index + start_index + 1, sublength);

                                call_stack.push({
                                    .pltext{subtext},
                                    .nested_tag_type{::pltxt2htm::NodeType::md_atx_h1},
                                });
                                goto restart;
                        }
                    }
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
                        u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len, color)) {
                    current_index += tag_len + 2;
                    // parsing start tag <color> successed
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        call_stack.push({
                            .pltext{::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1)},
                            .nested_tag_type{::pltxt2htm::NodeType::pl_color},
                        });
                        goto restart;
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len)) {
                    // parsing <del>$1</del>
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        call_stack.push({
                            .pltext{::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1)},
                            .nested_tag_type{::pltxt2htm::NodeType::html_del},
                        });
                        goto restart;
                    }
                    goto complete_parsing_tag;
                }
                // parsing: <discussion=$1>$2</discussion>
                ::fast_io::u8string id{};
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8'i', u8's', u8'c', u8'u', u8's', u8's',
                                                                   u8'i', u8'o', u8'n'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len, id)) {
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        call_stack.push({
                            .pltext{::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1)},
                            .nested_tag_type{::pltxt2htm::NodeType::pl_discussion},
                        });
                        goto restart;
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len, id)) {
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        call_stack.push({
                            .pltext{::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1)},
                            .nested_tag_type{::pltxt2htm::NodeType::pl_discussion},
                        });
                        goto restart;
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len)) {
                    // parsing html <h1> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_h1, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H1>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'2'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    // parsing html <h2> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_h2, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H2>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'3'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    // parsing html <h3> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_h3, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H3>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'4'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    // parsing html <h4> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_h4, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H4>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'5'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    // parsing html <h5> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_h5, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H5>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'6'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    // parsing html <h6> tag
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_h6, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::H6>(::std::move(subast)));
                    }
                    goto complete_parsing_tag;
                } else if (::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'r'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    current_index += tag_len + 2;
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Hr>());
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len)) {
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_i, ::std::addressof(current_index));
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len)) {
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_p, ::std::addressof(current_index));
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len, id_)) {
                    current_index += tag_len + 2;
                    auto id{::pltxt2htm::details::u8str2size_t(::fast_io::mnp::os_c_str(id_))};
                    if (!id.has_value()) {
                        goto not_valid_tag;
                    }

                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_size, ::std::addressof(current_index));
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
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len, id)) {
                    current_index += tag_len + 2;
                    if (current_index + 1 < pltext_size) {
                        // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                        // which does not make sense, can be opetimized(ignored) during parsing ast
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_user, ::std::addressof(current_index));
                        result.push_back(
                            ::pltxt2htm::details::HeapGuard<::pltxt2htm::User>(::std::move(subast), ::std::move(id)));
                    }
                    goto complete_parsing_tag;
                } else {
                    goto not_valid_tag;
                }
            }

            case u8'/': {
                switch (call_stack.top().nested_tag_type) {
                case ::pltxt2htm::NodeType::pl_color: {
#if __has_cpp_attribute(indeterminate)
                    ::std::size_t tag_len [[indeterminate]];
#else
                    ::std::size_t tag_len;
#endif
                    if (::pltxt2htm::details::try_parse_bare_tag<ndebug, u8'c', u8'o', u8'l', u8'o', u8'r'>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </color> successed
                    // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </a> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // Whether or not extern_index is out of range, extern for loop will handle it correctly.
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </b> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </a> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </p> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h1> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h2> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h3> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h4> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h5> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </h6> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        goto not_valid_tag;
                    }
                    // parsing end tag </del> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
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
            if ((chr & 0x80) == 0) {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                continue;
            } else if ((chr & 0xE0) == 0xC0) {
                if (current_index + 1 >= pltext_size) {
                    goto invalid_u8char;
                }
                auto next_char = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1);
                if ((next_char & 0xC0) != 0x80) {
                    goto invalid_u8char;
                }
                char32_t combine{static_cast<char32_t>(chr & 0x1F) << 6 | static_cast<char32_t>(next_char & 0x3F)};
                if (combine < 0x80 || combine > 0x7FF) {
                    goto invalid_u8char;
                }

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char});
                current_index += 1;
                continue;
            } else if ((chr & 0xF0) == 0xE0) {
                if (current_index + 2 >= pltext_size) {
                    goto invalid_u8char;
                }
                auto next_char = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1);
                if ((next_char & 0xC0) != 0x80) {
                    goto invalid_u8char;
                }
                auto next_char2 = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 2);
                if ((next_char2 & 0xC0) != 0x80) {
                    goto invalid_u8char;
                }
                char32_t combine{static_cast<char32_t>(chr & 0x0f) << 12 |
                                 static_cast<char32_t>(next_char & 0x3f) << 6 |
                                 static_cast<char32_t>(next_char2 & 0x3f)};
                if (combine < 0x800 || combine > 0xffff) {
                    goto invalid_u8char;
                }
                if (0xd800 <= combine && combine <= 0xdfff) {
                    goto invalid_u8char;
                }

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char2});
                current_index += 2;
                continue;
            } else if ((chr & 0xF8) == 0xF0) {
                if (current_index + 3 >= pltext_size) {
                    goto invalid_u8char;
                }
                auto next_char = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1);
                if ((next_char & 0xC0) != 0x80) {
                    goto invalid_u8char;
                }
                auto next_char2 = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 2);
                if ((next_char & 0xC0) != 0x80) {
                    goto invalid_u8char;
                }
                auto next_char3 = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 3);
                if ((next_char3 & 0xC0) != 0x80) {
                    goto invalid_u8char;
                }
                char32_t combine{
                    static_cast<char32_t>(chr & 0x07) << 18 | static_cast<char32_t>(next_char & 0x3F) << 12 |
                    static_cast<char32_t>(next_char2 & 0x3F) << 6 | static_cast<char32_t>(next_char3 & 0x3F)};
                if (combine < 0x10000 || combine > 0x10FFFF) {
                    goto invalid_u8char;
                }
                if (0xd800 <= combine && combine <= 0xdfff) {
                    goto invalid_u8char;
                }

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char2});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char3});
                current_index += 3;
                continue;
            }
        invalid_u8char:
            // Invalid utf-8 encoding
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
            continue;
        }
    }

    if (extern_index != nullptr) {
        *extern_index += pltext.size();
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
    ::fast_io::stack<::pltxt2htm::details::ParsePltxtFrameContext> call_stack;
    call_stack.push({
        .pltext{pltext},
        .nested_tag_type{::pltxt2htm::NodeType::base},
        .return_from_details{true},
    });
    auto&& current_index = call_stack.top().current_index;
    // Consider the following markdown
    // ```md
    // ## test
    // ```
    // Here, the first line is a header, will hit this case

#if __has_cpp_attribute(indeterminate)
    ::std::size_t end_index [[indeterminate]];
    ::std::size_t header_level [[indeterminate]];
#else
    ::std::size_t sublength;
    ::std::size_t header_level;
#endif
    // try parsing markdown atx header
    if (::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(pltext, current_index, sublength, header_level)) {
        ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
        if (current_index < pltext.size()) {
            auto subtext = ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index, sublength);
            call_stack.push({
                .pltext{subtext},
                .nested_tag_type{::pltxt2htm::NodeType::md_atx_h1},
                .return_from_details{true},
            });
            // passing ::pltxt2htm::NodeType::md_atx_h1 is incorrect but it's not used
            // because the sublength have already been calculated
            ::pltxt2htm::details::parse_pltxt<ndebug>(call_stack);
            auto subast_frame_index = call_stack.top().current_index;
            ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{::std::move(call_stack.top().subast)};
            call_stack.pop();
            call_stack.top().current_index += subast_frame_index;
            result.push_back(::pltxt2htm::details::switch_md_atx_header<ndebug>(header_level, ::std::move(subast)));
        } else {
            result.push_back(::pltxt2htm::details::switch_md_atx_header<ndebug>(header_level, {}));
        }
    }

    ::pltxt2htm::details::parse_pltxt<ndebug>(
        call_stack);
    for (auto&& node : call_stack.top().subast) {
        result.push_back(::std::move(node));
    }
    call_stack.pop();
    pltxt2htm_assert(call_stack.size() == 0, u8"call_stack size is not zero");
    return result;
}

} // namespace pltxt2htm

#include "pop_macro.hh"
