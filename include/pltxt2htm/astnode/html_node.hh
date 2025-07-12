#pragma once

#include "node_type.hh"
#include "basic.hh"

namespace pltxt2htm {

/**
 * @brief Represents '\n'.
 */
class LineBreak : public ::pltxt2htm::PlTxtNode {
public:
    constexpr LineBreak() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::line_break} {
    }

    constexpr LineBreak(::pltxt2htm::LineBreak const& other) noexcept = default;
    constexpr LineBreak(::pltxt2htm::LineBreak&& other) noexcept = default;
    constexpr ~LineBreak() noexcept = default;
    constexpr ::pltxt2htm::LineBreak& operator=(this ::pltxt2htm::LineBreak& self,
                                                ::pltxt2htm::LineBreak const& other) noexcept = delete;
    constexpr ::pltxt2htm::LineBreak& operator=(this ::pltxt2htm::LineBreak& self,
                                                ::pltxt2htm::LineBreak&& other) noexcept = delete;
};

/**
 * @brief Represents a <br> tag.
 */
class Br : public ::pltxt2htm::PlTxtNode {
public:
    constexpr Br() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::html_br} {
    }

    constexpr Br(::pltxt2htm::Br const& other) noexcept = default;
    constexpr Br(::pltxt2htm::Br&& other) noexcept = default;
    constexpr ~Br() noexcept = default;
    constexpr ::pltxt2htm::Br& operator=(::pltxt2htm::Br const& other) noexcept = delete;
    constexpr ::pltxt2htm::Br& operator=(::pltxt2htm::Br&& other) noexcept = delete;
};

/**
 * @brief Represents ` `.
 */
class Space : public ::pltxt2htm::PlTxtNode {
public:
    constexpr Space() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::space} {
    }

    constexpr Space(::pltxt2htm::Space const& other) noexcept = default;
    constexpr Space(::pltxt2htm::Space&& other) noexcept = default;
    constexpr ~Space() noexcept = default;
    constexpr ::pltxt2htm::Space& operator=(this ::pltxt2htm::Space& self,
                                            ::pltxt2htm::Space const& other) noexcept = delete;
    constexpr ::pltxt2htm::Space& operator=(this ::pltxt2htm::Space& self,
                                            ::pltxt2htm::Space&& other) noexcept = delete;
};

/**
 * @brief Represents `<`.
 */
class LessThan : public ::pltxt2htm::PlTxtNode {
public:
    constexpr LessThan() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::less_than} {
    }

    constexpr LessThan(::pltxt2htm::LessThan const& other) noexcept = default;
    constexpr LessThan(::pltxt2htm::LessThan&& other) noexcept = default;
    constexpr ~LessThan() noexcept = default;
    constexpr ::pltxt2htm::LessThan& operator=(this ::pltxt2htm::LessThan& self,
                                               ::pltxt2htm::LessThan const& other) noexcept = delete;
    constexpr ::pltxt2htm::LessThan& operator=(this ::pltxt2htm::LessThan& self,
                                               ::pltxt2htm::LessThan&& other) noexcept = delete;
};

/**
 * @brief Represents `>`.
 */
class GreaterThan : public ::pltxt2htm::PlTxtNode {
public:
    constexpr GreaterThan() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::greater_than} {
    }

    constexpr GreaterThan(::pltxt2htm::GreaterThan const& other) noexcept = default;
    constexpr GreaterThan(::pltxt2htm::GreaterThan&& other) noexcept = default;
    constexpr ~GreaterThan() noexcept = default;
    constexpr ::pltxt2htm::GreaterThan& operator=(this ::pltxt2htm::GreaterThan& self,
                                                  ::pltxt2htm::GreaterThan const& other) noexcept = delete;
    constexpr ::pltxt2htm::GreaterThan& operator=(this ::pltxt2htm::GreaterThan& self,
                                                  ::pltxt2htm::GreaterThan&& other) noexcept = delete;
};

/**
 * @brief Represents `&`.
 */
class Ampersand : public ::pltxt2htm::PlTxtNode {
public:
    constexpr Ampersand() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::ampersand} {
    }

    constexpr Ampersand(::pltxt2htm::Ampersand const& other) noexcept = default;
    constexpr Ampersand(::pltxt2htm::Ampersand&& other) noexcept = default;
    constexpr ~Ampersand() noexcept = default;
    constexpr ::pltxt2htm::Ampersand& operator=(this ::pltxt2htm::Ampersand& self,
                                                ::pltxt2htm::Ampersand const& other) noexcept = delete;
    constexpr ::pltxt2htm::Ampersand& operator=(this ::pltxt2htm::Ampersand& self,
                                                ::pltxt2htm::Ampersand&& other) noexcept = delete;
};

/**
 * @brief Represents `'`.
 */
class SingleQuotationMark : public ::pltxt2htm::PlTxtNode {
public:
    constexpr SingleQuotationMark() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::single_quote} {
    }

    constexpr SingleQuotationMark(::pltxt2htm::SingleQuotationMark const& other) noexcept = default;
    constexpr SingleQuotationMark(::pltxt2htm::SingleQuotationMark&& other) noexcept = default;
    constexpr ~SingleQuotationMark() noexcept = default;
    constexpr ::pltxt2htm::SingleQuotationMark& operator=(::pltxt2htm::SingleQuotationMark const& other) noexcept =
        delete;
    constexpr ::pltxt2htm::SingleQuotationMark& operator=(::pltxt2htm::SingleQuotationMark&&) noexcept = delete;
};

/**
 * @brief Represents `"`.
 */
class DoubleQuotationMark : public ::pltxt2htm::PlTxtNode {
public:
    constexpr DoubleQuotationMark() noexcept
        : ::pltxt2htm::PlTxtNode{::pltxt2htm::NodeType::double_quote} {
    }

    constexpr DoubleQuotationMark(::pltxt2htm::DoubleQuotationMark const&) noexcept = default;
    constexpr DoubleQuotationMark(::pltxt2htm::DoubleQuotationMark&&) noexcept = default;
    constexpr ~DoubleQuotationMark() noexcept = default;
    constexpr ::pltxt2htm::DoubleQuotationMark& operator=(this ::pltxt2htm::DoubleQuotationMark& self,
                                                          ::pltxt2htm::DoubleQuotationMark const&) noexcept = delete;
    constexpr ::pltxt2htm::DoubleQuotationMark& operator=(this ::pltxt2htm::DoubleQuotationMark& self,
                                                          ::pltxt2htm::DoubleQuotationMark&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<h1>`.
 */
class H1 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr H1() noexcept = delete;

    constexpr H1(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h1, ::std::move(subast)} {
    }

    constexpr H1(::pltxt2htm::H1 const&) noexcept = default;

    constexpr H1(::pltxt2htm::H1&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h1, ::std::move(other.subast_)} {
    }

    constexpr ~H1() noexcept = default;

    constexpr ::pltxt2htm::H1& operator=(::pltxt2htm::H1 const&) noexcept = delete;

    constexpr ::pltxt2htm::H1& operator=(::pltxt2htm::H1&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<h2>`.
 */
class H2 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr H2() noexcept = delete;

    constexpr H2(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h2, ::std::move(subast)} {
    }

    constexpr H2(::pltxt2htm::H2 const&) noexcept = default;

    constexpr H2(::pltxt2htm::H2&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h2, ::std::move(other.subast_)} {
    }

    constexpr ~H2() noexcept = default;

    constexpr ::pltxt2htm::H2& operator=(::pltxt2htm::H2 const&) noexcept = delete;

    constexpr ::pltxt2htm::H2& operator=(::pltxt2htm::H2&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<h3>`.
 */
class H3 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr H3() noexcept = delete;

    constexpr H3(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h3, ::std::move(subast)} {
    }

    constexpr H3(::pltxt2htm::H3 const&) noexcept = default;

    constexpr H3(::pltxt2htm::H3&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h3, ::std::move(other.subast_)} {
    }

    constexpr ~H3() noexcept = default;

    constexpr ::pltxt2htm::H3& operator=(::pltxt2htm::H3 const&) noexcept = delete;

    constexpr ::pltxt2htm::H3& operator=(::pltxt2htm::H3&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<h4>`.
 */
class H4 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr H4() noexcept = delete;

    constexpr H4(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h4, ::std::move(subast)} {
    }

    constexpr H4(::pltxt2htm::H4 const&) noexcept = default;

    constexpr H4(::pltxt2htm::H4&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h4, ::std::move(other.subast_)} {
    }

    constexpr ~H4() noexcept = default;

    constexpr ::pltxt2htm::H4& operator=(::pltxt2htm::H4 const&) noexcept = delete;

    constexpr ::pltxt2htm::H4& operator=(::pltxt2htm::H4&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<h5>`.
 */
class H5 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr H5() noexcept = delete;

    constexpr H5(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h5, ::std::move(subast)} {
    }

    constexpr H5(::pltxt2htm::H5 const&) noexcept = default;

    constexpr H5(::pltxt2htm::H5&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h5, ::std::move(other.subast_)} {
    }

    constexpr ~H5() noexcept = default;

    constexpr ::pltxt2htm::H5& operator=(::pltxt2htm::H5 const&) noexcept = delete;

    constexpr ::pltxt2htm::H5& operator=(::pltxt2htm::H5&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<h6>`.
 */
class H6 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr H6() noexcept = delete;

    constexpr H6(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h6, ::std::move(subast)} {
    }

    constexpr H6(::pltxt2htm::H6 const&) noexcept = default;

    constexpr H6(::pltxt2htm::H6&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h6, ::std::move(other.subast_)} {
    }

    constexpr ~H6() noexcept = default;

    constexpr ::pltxt2htm::H6& operator=(::pltxt2htm::H6 const&) noexcept = delete;

    constexpr ::pltxt2htm::H6& operator=(::pltxt2htm::H6&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<p>`.
 */
class P : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr P() noexcept = delete;

    constexpr P(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_p, ::std::move(subast)} {
    }

    constexpr P(::pltxt2htm::P const&) noexcept = default;

    constexpr P(::pltxt2htm::P&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_p, ::std::move(other.subast_)} {
    }

    constexpr ~P() noexcept = default;

    constexpr ::pltxt2htm::P& operator=(::pltxt2htm::P const&) noexcept = delete;

    constexpr ::pltxt2htm::P& operator=(::pltxt2htm::P&&) noexcept = delete;
};

/**
 * @brief Represents HTML `<del>`.
 */
class Del : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr Del() noexcept = delete;

    constexpr Del(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_del, ::std::move(subast)} {
    }

    constexpr Del(::pltxt2htm::Del const&) noexcept = default;

    constexpr Del(::pltxt2htm::Del&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_del, ::std::move(other.subast_)} {
    }

    constexpr ~Del() noexcept = default;

    constexpr ::pltxt2htm::Del& operator=(::pltxt2htm::Del const&) noexcept = delete;

    constexpr ::pltxt2htm::Del& operator=(::pltxt2htm::Del&&) noexcept = delete;
};

} // namespace pltxt2htm
