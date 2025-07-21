#pragma once

#include <cstddef>
#include <memory>
#include <fast_io/fast_io_dsal/stack.h>
#include <fast_io/fast_io_dsal/vector.h>
#include <fast_io/fast_io_dsal/string.h>
#include <fast_io/fast_io_dsal/string_view.h>
#include <exception/exception.hh>
#include "utils.hh"
#include "heap_guard.hh"
#include "astnode/node_type.hh"
#include "astnode/basic.hh"
#include "astnode/html_node.hh"
#include "astnode/markdown_node.hh"
#include "astnode/physics_lab_node.hh"
#include "push_macro.hh"

namespace pltxt2htm {

namespace details {

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
 * @param[out] sublength_: length of the heading context
 * @param[out] md_atx_heading_type_: md_atx_h1 ~ md_atx_h6
 * @return true if the parsing is successful
 */
template<bool ndebug>
[[nodiscard]]
constexpr bool try_parse_md_atx_heading(::fast_io::u8string_view pltext, ::std::size_t& start_index_,
                                        ::std::size_t& sublength_, ::pltxt2htm::NodeType& md_atx_heading_type_)
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
    ::std::size_t md_atx_heading_type{static_cast<::std::size_t>(::pltxt2htm::NodeType::md_atx_h1) - 1};
    while (true) {
        if (start_index >= pltext_size) {
            // https://spec.commonmark.org/0.31.2/#example-79
            if (static_cast<::std::size_t>(::pltxt2htm::NodeType::md_atx_h1) <= md_atx_heading_type && md_atx_heading_type <= static_cast<::std::size_t>(::pltxt2htm::NodeType::md_atx_h6)) {
                start_index_ = start_index;
                sublength_ = 0;
                md_atx_heading_type_ = static_cast<::pltxt2htm::NodeType>(md_atx_heading_type);
                return true;
            } else {
                return false;
            }
        }
        if (::pltxt2htm::details::u8string_view_index<ndebug>(pltext, start_index) != u8'#') {
            break;
        }
        ++start_index;
        ++md_atx_heading_type;
    }
    if (md_atx_heading_type < static_cast<::std::size_t>(::pltxt2htm::NodeType::md_atx_h1) || static_cast<::std::size_t>(::pltxt2htm::NodeType::md_atx_h6) < md_atx_heading_type ||
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
            md_atx_heading_type_ = static_cast<::pltxt2htm::NodeType>(md_atx_heading_type);
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
    md_atx_heading_type_ = static_cast<::pltxt2htm::NodeType>(md_atx_heading_type);
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
::pltxt2htm::NodeType md_atx_heading_type,
    ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
    -> ::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode> {
    switch (md_atx_heading_type) {
    case ::pltxt2htm::NodeType::md_atx_h1: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH1>(::std::move(subast));
    }
    case ::pltxt2htm::NodeType::md_atx_h2: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH2>(::std::move(subast));
    }
    case ::pltxt2htm::NodeType::md_atx_h3: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH3>(::std::move(subast));
    }
    case ::pltxt2htm::NodeType::md_atx_h4: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH4>(::std::move(subast));
    }
    case ::pltxt2htm::NodeType::md_atx_h5: {
        return ::pltxt2htm::details::HeapGuard<::pltxt2htm::AtxH5>(::std::move(subast));
    }
    case ::pltxt2htm::NodeType::md_atx_h6: {
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

class BasicFrameContext {
public:
    ::fast_io::u8string_view pltext;
    ::pltxt2htm::NodeType const nested_tag_type;
    bool const return_from_recursion{false};
    ::std::size_t current_index{};
    ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};

protected:
    constexpr BasicFrameContext(::fast_io::u8string_view pltext_, ::pltxt2htm::NodeType const nested_tag_type_,
                                bool const return_from_recursion_) noexcept
        : pltext(pltext_),
          nested_tag_type{nested_tag_type_},
          return_from_recursion{return_from_recursion_} {
    }
};

class BareTagContext : public ::pltxt2htm::details::BasicFrameContext {
public:
    constexpr BareTagContext(::fast_io::u8string_view pltext_, ::pltxt2htm::NodeType const nested_tag_type_,
                             bool const return_from_recursion_ = false) noexcept
        : ::pltxt2htm::details::BasicFrameContext(pltext_, nested_tag_type_, return_from_recursion_) {
    }
};

class EqualSignTagContext : public ::pltxt2htm::details::BasicFrameContext {
public:
    ::fast_io::u8string id;

    constexpr EqualSignTagContext(::fast_io::u8string_view pltext_, ::pltxt2htm::NodeType const nested_tag_type_,
                                  ::fast_io::u8string&& id_, bool const return_from_recursion_ = false) noexcept
        : ::pltxt2htm::details::BasicFrameContext{pltext_, nested_tag_type_, return_from_recursion_},
          id(::std::move(id_)) {
    }
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
[[nodiscard]]
constexpr auto parse_pltxt(
    ::fast_io::stack<::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BasicFrameContext>,
                     ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BasicFrameContext>>>&
        call_stack)
#if __cpp_exceptions < 199711L
    noexcept
#endif
    -> ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> {
restart:
    auto&& current_index = call_stack.top()->current_index;
    auto&& pltext = call_stack.top()->pltext;
    auto&& result = call_stack.top()->subast;
    ::std::size_t const pltext_size{pltext.size()};

    for (; current_index < pltext_size; ++current_index) {
        char8_t const chr{::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index)};

        if (chr == u8'\n') {
            result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LineBreak>{});

#if __has_cpp_attribute(indeterminate)
            ::std::size_t start_index [[indeterminate]];
            ::std::size_t end_index [[indeterminate]];
            ::pltxt2htm::NodeType md_atx_heading_type [[indeterminate]];
#else
            ::std::size_t start_index;
            ::std::size_t sublength;
            ::pltxt2htm::NodeType md_atx_heading_type;
#endif
            // try parsing markdown atx header
            if (current_index + 1 < pltext_size &&
                ::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(
                    ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1), start_index,
                    sublength, md_atx_heading_type)) {
                ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
                if (current_index + start_index + 1 < pltext_size) {
                    auto subtext = ::pltxt2htm::details::u8string_view_subview<ndebug>(
                        pltext, current_index + start_index + 1, sublength);
                    call_stack.push(::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BareTagContext>(
                        subtext, md_atx_heading_type));
                    goto restart;
                }
                current_index += start_index;
                result.push_back(::pltxt2htm::details::switch_md_atx_header<ndebug>(md_atx_heading_type, ::std::move(subast)));
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
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                continue;
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
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_a, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::A>(::std::move(subast)));
                    }
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_b, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::B>(::std::move(subast)));
                    }
                    continue;
                } else if (::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'r'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    current_index += tag_len + 2;
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Br>(::pltxt2htm::Br()));

#if __has_cpp_attribute(indeterminate)
                    ::std::size_t start_index [[indeterminate]];
                    ::std::size_t end_index [[indeterminate]];
                    ::pltxt2htm::NodeType md_atx_heading_type [[indeterminate]];
#else
                    ::std::size_t start_index;
                    ::std::size_t sublength;
                    ::pltxt2htm::NodeType md_atx_heading_type;
#endif
                    // try parsing markdown atx header
                    if (current_index + 1 < pltext_size &&
                        ::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1), start_index,
                            sublength, md_atx_heading_type)) {
                        ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
                        if (current_index + start_index + 1 < pltext_size) {
                            auto subtext = ::pltxt2htm::details::u8string_view_subview<ndebug>(
                                pltext, current_index + start_index + 1, sublength);

                            subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                                subtext, md_atx_heading_type, ::std::addressof(current_index));
                        }
                        current_index += start_index;
                        result.push_back(
                            ::pltxt2htm::details::switch_md_atx_header<ndebug>(md_atx_heading_type, ::std::move(subast)));
                    }
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_color, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Color>(::std::move(subast),
                                                                                             ::std::move(color)));
                    }
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::html_del, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Del>(::std::move(subast)));
                    }
                    continue;
                }
                // parsing: <discussion=$1>$2</discussion>
                ::fast_io::u8string id{};
                if (::pltxt2htm::details::try_parse_equal_sign_tag<ndebug, u8'i', u8's', u8'c', u8'u', u8's', u8's',
                                                                   u8'i', u8'o', u8'n'>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len, id)) {
                    current_index += tag_len + 2;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
                }

                if (current_index + 1 < pltext_size) {
                    // if forward_index + 1 >= pltext_size, it means that a not closed tag in the end of the text
                    // which does not make sense, can be opetimized(ignored) during parsing ast
                    auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                        ::pltxt2htm::NodeType::pl_discussion, ::std::addressof(current_index));
                    result.push_back(
                        ::pltxt2htm::details::HeapGuard<::pltxt2htm::Discussion>(::std::move(subast), ::std::move(id)));
                }
                continue;
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
                        auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 1),
                            ::pltxt2htm::NodeType::pl_experiment, ::std::addressof(current_index));
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Experiment>(::std::move(subast),
                                                                                                  ::std::move(id)));
                    }
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                    continue;
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
                    continue;
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
                    continue;
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
                    continue;
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
                    continue;
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
                    continue;
                } else if (::pltxt2htm::details::try_parse_self_closing_tag<ndebug, u8'r'>(
                               ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2),
                               tag_len)) {
                    current_index += tag_len + 2;
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::Hr>());
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                    continue;
                } else {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
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
                            ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, current_index + 2), tag_len) ==
                        false) {
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
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
                        result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                        continue;
                    }
                    // parsing end tag </del> successed
                    if (extern_index != nullptr) {
                        *extern_index += current_index + tag_len + 3;
                    }
                    return result;
                }
                default:
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                    continue;
                }
                ::exception::unreachable<ndebug>();
            }

            default: {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::LessThan>{});
                continue;
            }
            }

            ::exception::unreachable<ndebug>();
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
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                auto next_char = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1);
                if ((next_char & 0xC0) != 0x80) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                char32_t combine{static_cast<char32_t>(chr & 0x1F) << 6 | static_cast<char32_t>(next_char & 0x3F)};
                if (combine < 0x80 || combine > 0x7FF) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char});
                current_index += 1;
                continue;
            } else if ((chr & 0xF0) == 0xE0) {
                if (current_index + 2 >= pltext_size) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                auto next_char = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1);
                if ((next_char & 0xC0) != 0x80) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                auto next_char2 = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 2);
                if ((next_char2 & 0xC0) != 0x80) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                char32_t combine{static_cast<char32_t>(chr & 0x0f) << 12 |
                                 static_cast<char32_t>(next_char & 0x3f) << 6 |
                                 static_cast<char32_t>(next_char2 & 0x3f)};
                if (combine < 0x800 || combine > 0xffff) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                if (0xd800 <= combine && combine <= 0xdfff) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char2});
                current_index += 2;
                continue;
            } else if ((chr & 0xF8) == 0xF0) {
                if (current_index + 3 >= pltext_size) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                auto next_char = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 1);
                if ((next_char & 0xC0) != 0x80) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                auto next_char2 = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 2);
                if ((next_char & 0xC0) != 0x80) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                auto next_char3 = ::pltxt2htm::details::u8string_view_index<ndebug>(pltext, current_index + 3);
                if ((next_char3 & 0xC0) != 0x80) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                char32_t combine{
                    static_cast<char32_t>(chr & 0x07) << 18 | static_cast<char32_t>(next_char & 0x3F) << 12 |
                    static_cast<char32_t>(next_char2 & 0x3F) << 6 | static_cast<char32_t>(next_char3 & 0x3F)};
                if (combine < 0x10000 || combine > 0x10FFFF) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }
                if (0xd800 <= combine && combine <= 0xdfff) {
                    result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                    continue;
                }

                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{chr});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char2});
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::U8Char>{next_char3});
                current_index += 3;
                continue;
            } else {
                result.push_back(::pltxt2htm::details::HeapGuard<::pltxt2htm::InvalidU8Char>{});
                continue;
            }
        }
    }

    if (extern_index != nullptr) {
        *extern_index += pltext_size;
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
    -> ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> {
    // fast_io::deque contains bug about RAII
    ::fast_io::stack<::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BasicFrameContext>,
                     ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BasicFrameContext>>>
        call_stack{};
    ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> result{};

    // Consider the following markdown
    // ```md
    // ## test
    // ```
    // Here, the first line is a header, will hit this case
    ::std::size_t start_index{};
#if __has_cpp_attribute(indeterminate)
    ::std::size_t end_index [[indeterminate]];
    ::pltxt2htm::NodeType md_atx_heading_type [[indeterminate]];
#else
    ::std::size_t sublength;
    ::pltxt2htm::NodeType md_atx_heading_type;
#endif
    // try parsing markdown atx header
    if (::pltxt2htm::details::try_parse_md_atx_heading<ndebug>(pltext, start_index, sublength, md_atx_heading_type)) {
        ::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> subast{};
        if (start_index < pltext.size()) {
            auto subtext = ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, start_index, sublength);
            call_stack.push(::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BareTagContext>(
                subtext, md_atx_heading_type, true));
            subast = ::pltxt2htm::details::parse_pltxt<ndebug>(call_stack);
        }
        result.push_back(::pltxt2htm::details::switch_md_atx_header<ndebug>(md_atx_heading_type, ::std::move(subast)));
    }

    // other common cases
    call_stack.push(::pltxt2htm::details::HeapGuard<::pltxt2htm::details::BareTagContext>(
        ::pltxt2htm::details::u8string_view_subview<ndebug>(pltext, start_index), ::pltxt2htm::NodeType::base, true));
    auto subast = ::pltxt2htm::details::parse_pltxt<ndebug>(call_stack);
    for (auto&& node : subast) {
        result.push_back(::std::move(node));
    }
    return result;
}

} // namespace pltxt2htm

#include "pop_macro.hh"
