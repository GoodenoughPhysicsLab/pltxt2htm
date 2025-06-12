#pragma once

#include <fast_io/fast_io_dsal/vector.h>
#include <fast_io/fast_io_dsal/string.h>
#include <fast_io/fast_io_dsal/string_view.h>
#include <exception/exception.hh>
#include "parser.hh"
#include "backend.hh"

namespace pltxt2htm {

/**
 * @brief Convert Quantum Physics (aka. Physics-Lab, pl) text to HTML.
 *        Supported syntax are listed in pltxt2htm/astnode.hh: `enum class NodeType`
 * @tparam ndebug: show explanation in README.md Q/A
 * @param pltext The text of Quantum Physics.
 */
template<::pltxt2htm::BackendText backend_text = ::pltxt2htm::BackendText::advanced_html, bool ndebug = false>
[[nodiscard]]
constexpr auto pltxt2html(::fast_io::u8string_view pltext,
                          ::fast_io::u8string_view host)
#ifndef PLTXT2HTM_ENABLE_PRINT
    noexcept
#endif
{
    return ::pltxt2htm::ast2html<backend_text, ndebug>(
        ::pltxt2htm::parse_pltxt<ndebug>(pltext), host);
}

} // namespace pltxt2htm
