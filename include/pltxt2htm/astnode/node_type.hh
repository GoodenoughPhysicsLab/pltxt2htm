#pragma once

#include <cstdint>

namespace pltxt2htm {

/**
 * @brief ast node type of Quantum-Physics (aka. Physics-Lab, pl) text
 * @details show details in astnode.hh, every AstNode owns a NodeType
 */
enum class NodeType : ::std::uint_least32_t {
    base = 0,
    // a utf-8 char/string
    u8char,

    // \n
    line_break,
    // space will be squashed by html render
    space,
    // &
    ampersand,
    // "
    double_quote,
    // '
    single_quote,
    // <
    less_than,
    // >
    greater_than,

    // Quantum-Physics's <color> tag
    pl_color,
    // Quantum-Physics's <a> tag
    pl_a,
    // Quantum-Physics's <experiment> tag
    pl_experiment,
    // Quantum-Physics's <discussion> tag
    pl_discussion,
    // Quantum-Physics's <user> tag
    pl_user,
    // Quantum-Physics's <size> tag
    pl_size,

    // bold
    pl_b, // be used by Quantum-Physics, html, markdown(**)
    // italic
    pl_i, // be used by Quantum-Physics, html, markdown(*)

    // paragraph
    html_p, // be used by html, markdown(\n\n)
    // HTML's <br> tag
    html_br,
    // html header tags
    html_h1,
    html_h2,
    html_h3,
    html_h4,
    html_h5,
    html_h6,
    // html <del> tag
    html_del,

    // markdown headers
    md_atx_h1,
    md_atx_h2,
    md_atx_h3,
    md_atx_h4,
    md_atx_h5,
    md_atx_h6,

    // latex
    // latex,
};

} // namespace pltxt2htm
