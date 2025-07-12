#pragma once

#include <utility>
#include "basic.hh"

namespace pltxt2htm {

/**
 * @brief Represents markdown atx `#` header.
 */
class AtxH1 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr AtxH1() noexcept = delete;

    constexpr AtxH1(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h1, ::std::move(subast)} {
    }

    constexpr AtxH1(::pltxt2htm::AtxH1 const&) noexcept = default;

    constexpr AtxH1(::pltxt2htm::AtxH1&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h1, ::std::move(other.subast_)} {
    }

    constexpr ~AtxH1() noexcept = default;

    constexpr ::pltxt2htm::AtxH1& operator=(::pltxt2htm::AtxH1 const&) noexcept = delete;

    constexpr ::pltxt2htm::AtxH1& operator=(::pltxt2htm::AtxH1&&) noexcept = delete;
};

/**
 * @brief Represents markdown atx `##` header.
 */
class AtxH2 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr AtxH2() noexcept = delete;

    constexpr AtxH2(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h2, ::std::move(subast)} {
    }

    constexpr AtxH2(::pltxt2htm::AtxH2 const&) noexcept = default;

    constexpr AtxH2(::pltxt2htm::AtxH2&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h2, ::std::move(other.subast_)} {
    }

    constexpr ~AtxH2() noexcept = default;

    constexpr ::pltxt2htm::AtxH2& operator=(::pltxt2htm::AtxH2 const&) noexcept = delete;

    constexpr ::pltxt2htm::AtxH2& operator=(::pltxt2htm::AtxH2&&) noexcept = delete;
};

/**
 * @brief Represents markdown atx `###` header.
 */
class AtxH3 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr AtxH3() noexcept = delete;

    constexpr AtxH3(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h3, ::std::move(subast)} {
    }

    constexpr AtxH3(::pltxt2htm::AtxH3 const&) noexcept = default;

    constexpr AtxH3(::pltxt2htm::AtxH3&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h3, ::std::move(other.subast_)} {
    }

    constexpr ~AtxH3() noexcept = default;

    constexpr ::pltxt2htm::AtxH3& operator=(::pltxt2htm::AtxH3 const&) noexcept = delete;

    constexpr ::pltxt2htm::AtxH3& operator=(::pltxt2htm::AtxH3&&) noexcept = delete;
};

/**
 * @brief Represents markdown atx `####` header.
 */
class AtxH4 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr AtxH4() noexcept = delete;

    constexpr AtxH4(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h4, ::std::move(subast)} {
    }

    constexpr AtxH4(::pltxt2htm::AtxH4 const&) noexcept = default;

    constexpr AtxH4(::pltxt2htm::AtxH4&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h4, ::std::move(other.subast_)} {
    }

    constexpr ~AtxH4() noexcept = default;

    constexpr ::pltxt2htm::AtxH4& operator=(::pltxt2htm::AtxH4 const&) noexcept = delete;

    constexpr ::pltxt2htm::AtxH4& operator=(::pltxt2htm::AtxH4&&) noexcept = delete;
};

/**
 * @brief Represents markdown atx `#####` header.
 */
class AtxH5 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr AtxH5() noexcept = delete;

    constexpr AtxH5(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h5, ::std::move(subast)} {
    }

    constexpr AtxH5(::pltxt2htm::AtxH5 const&) noexcept = default;

    constexpr AtxH5(::pltxt2htm::AtxH5&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h5, ::std::move(other.subast_)} {
    }

    constexpr ~AtxH5() noexcept = default;

    constexpr ::pltxt2htm::AtxH5& operator=(::pltxt2htm::AtxH5 const&) noexcept = delete;

    constexpr ::pltxt2htm::AtxH5& operator=(::pltxt2htm::AtxH5&&) noexcept = delete;
};

/**
 * @brief Represents markdown atx `######` header.
 */
class AtxH6 : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr AtxH6() noexcept = delete;

    constexpr AtxH6(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h6, ::std::move(subast)} {
    }

    constexpr AtxH6(::pltxt2htm::AtxH6 const&) noexcept = default;

    constexpr AtxH6(::pltxt2htm::AtxH6&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::html_h6, ::std::move(other.subast_)} {
    }

    constexpr ~AtxH6() noexcept = default;

    constexpr ::pltxt2htm::AtxH6& operator=(::pltxt2htm::AtxH6 const&) noexcept = delete;

    constexpr ::pltxt2htm::AtxH6& operator=(::pltxt2htm::AtxH6&&) noexcept = delete;
};

} // namespace pltxt2htm
