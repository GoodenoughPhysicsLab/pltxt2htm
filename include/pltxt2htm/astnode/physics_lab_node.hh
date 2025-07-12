#pragma once

/**
 * @file plext.hh
 * @brief Quantum-Physics's text extensions
 */

#include <utility>
#include <fast_io/fast_io_dsal/string.h>
#include <exception/exception.hh>
#include "basic.hh"
#include "../heap_guard.hh"

namespace pltxt2htm {

/**
 * @brief Color node
 * @example - <color=xxx>...</color>
 *          - <Color=xxx>...</Color>
 *          - <COLOR=xxx>...</COLOR>
 */
class Color : public ::pltxt2htm::details::PairedTagBase {
    ::fast_io::u8string color_;

public:
    constexpr Color() noexcept = delete;

    constexpr Color(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& text,
                    ::fast_io::u8string&& color) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_color, ::std::move(text)},
          color_(::std::move(color)) {
    }

    constexpr Color(::pltxt2htm::Color const&) noexcept = default;

    constexpr Color(::pltxt2htm::Color&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_color, ::std::move(other.subast_)},
          color_(::std::move(other.color_)) {
    }

    constexpr ~Color() noexcept = default;

    constexpr ::pltxt2htm::Color& operator=(::pltxt2htm::Color const&) noexcept = delete;

    constexpr ::pltxt2htm::Color& operator=(::pltxt2htm::Color&&) noexcept = delete;

    [[nodiscard]]
    constexpr auto&& get_color(this auto&& self) noexcept {
        return ::std::forward_like<decltype(self)>(self.color_);
    }
};

class A : public ::pltxt2htm::details::PairedTagBase {
    ::fast_io::u8string color_;

public:
    constexpr A() noexcept = delete;

    constexpr A(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& text) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_a, ::std::move(text)},
          color_{u8"#0000AA"} {
    }

    constexpr A(::pltxt2htm::A const&) noexcept = default;

    constexpr A(::pltxt2htm::A&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_a, ::std::move(other.subast_)},
          color_{u8"#0000AA"} {
    }

    constexpr ~A() noexcept = default;

    constexpr ::pltxt2htm::A& operator=(::pltxt2htm::A const&) noexcept = delete;

    /**
     * @brief move assignment operator
     * @note assign to a rvalue is not allowed
     */
    constexpr ::pltxt2htm::A& operator=(::pltxt2htm::A&&) noexcept = delete;

    [[nodiscard]]
    constexpr auto&& get_color(this auto&& self) noexcept {
        return ::std::forward_like<decltype(self)>(self.color_);
    }
};

/**
 * @brief Experiment node
 * @example - <Experiment=xxx>...</Experiment>
 *          - <experiment=xxx>...</experiment>
 *          - <EXPERIMENT=xxx>...</EXPERIMENT>
 */
class Experiment : public ::pltxt2htm::details::PairedTagBase {
    ::fast_io::u8string id_;

public:
    constexpr Experiment() noexcept = delete;

    constexpr Experiment(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast,
                         ::fast_io::u8string&& id) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_experiment, ::std::move(subast)},
          id_(::std::move(id)) {
    }

    constexpr Experiment(::pltxt2htm::Experiment const&) noexcept = default;

    constexpr Experiment(::pltxt2htm::Experiment&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_experiment, ::std::move(other.subast_)},
          id_(::std::move(other.id_)) {
    }

    constexpr ~Experiment() noexcept = default;

    constexpr ::pltxt2htm::Experiment& operator=(this ::pltxt2htm::Experiment& self,
                                                 ::pltxt2htm::Experiment const&) noexcept = delete;

    constexpr ::pltxt2htm::Experiment& operator=(this ::pltxt2htm::Experiment& self,
                                                 ::pltxt2htm::Experiment&&) noexcept = delete;

    [[nodiscard]]
    constexpr auto&& get_id(this auto&& self) noexcept {
        return ::std::forward_like<decltype(self)>(self.id_);
    }
};

/**
 * @brief Discussion node
 * @example - <Discussion=xxx>...</Discussion>
 *          - <discussion=xxx>...</discussion>
 *          - <DISCUSSION=xxx>...</DISCUSSION>
 */
class Discussion : public ::pltxt2htm::details::PairedTagBase {
    ::fast_io::u8string id_{};

public:
    constexpr Discussion() noexcept = delete;

    constexpr Discussion(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast,
                         ::fast_io::u8string&& id) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_discussion, ::std::move(subast)},
          id_(::std::move(id)) {
    }

    constexpr Discussion(::pltxt2htm::Discussion const&) noexcept = default;

    constexpr Discussion(::pltxt2htm::Discussion&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_discussion, ::std::move(other.subast_)},
          id_(::std::move(other.id_)) {
    }

    constexpr ~Discussion() noexcept = default;

    constexpr ::pltxt2htm::Discussion& operator=(this ::pltxt2htm::Discussion& self,
                                                 ::pltxt2htm::Discussion const&) noexcept = delete;

    constexpr ::pltxt2htm::Discussion& operator=(this ::pltxt2htm::Discussion& self,
                                                 ::pltxt2htm::Discussion&&) noexcept = delete;

    [[nodiscard]]
    constexpr auto&& get_id(this auto&& self) noexcept {
        return ::std::forward_like<decltype(self)>(self.id_);
    }
};

/**
 * @brief User node
 * @example - <user=xxx>...</user>
 */
class User : public ::pltxt2htm::details::PairedTagBase {
    ::fast_io::u8string id_{};

public:
    constexpr User() noexcept = delete;

    constexpr User(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast,
                   ::fast_io::u8string&& id) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_user, ::std::move(subast)},
          id_(::std::move(id)) {
    }

    constexpr User(::pltxt2htm::User const&) noexcept = default;

    constexpr User(::pltxt2htm::User&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_user, ::std::move(other.subast_)},
          id_(::std::move(other.id_)) {
    }

    constexpr ~User() noexcept = default;

    constexpr ::pltxt2htm::User& operator=(::pltxt2htm::User const&) noexcept = delete;

    constexpr ::pltxt2htm::User& operator=(::pltxt2htm::User&&) noexcept = delete;

    [[nodiscard]]
    constexpr auto&& get_id(this auto&& self) noexcept {
        return ::std::forward_like<decltype(self)>(self.id_);
    }
};

/**
 * @brief Size node
 * @example - <size=xxx>...</size>
 */
class Size : public ::pltxt2htm::details::PairedTagBase {
    ::std::size_t id_{};

public:
    constexpr Size() noexcept = delete;

    constexpr Size(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast,
                   ::std::size_t id) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_size, ::std::move(subast)},
          id_(::std::move(id)) {
    }

    constexpr Size(::pltxt2htm::Size const&) noexcept = default;

    constexpr Size(::pltxt2htm::Size&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_size, ::std::move(other.subast_)},
          id_(::std::move(other.id_)) {
    }

    constexpr ~Size() noexcept = default;

    constexpr ::pltxt2htm::Size& operator=(::pltxt2htm::Size const&) noexcept = delete;

    constexpr ::pltxt2htm::Size& operator=(::pltxt2htm::Size&&) noexcept = delete;

    [[nodiscard]]
    constexpr auto&& get_id(this auto&& self) noexcept {
        return ::std::forward_like<decltype(self)>(self.id_);
    }
};

class I : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr I() noexcept = delete;

    constexpr I(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_i, ::std::move(subast)} {
    }

    constexpr I(::pltxt2htm::I const&) noexcept = default;

    constexpr I(::pltxt2htm::I&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_i, ::std::move(other.subast_)} {
    }

    constexpr ~I() noexcept = default;

    constexpr ::pltxt2htm::I& operator=(::pltxt2htm::I const&) noexcept = delete;

    constexpr ::pltxt2htm::I& operator=(::pltxt2htm::I&&) noexcept = delete;
};

class B : public ::pltxt2htm::details::PairedTagBase {
public:
    constexpr B() noexcept = delete;

    constexpr B(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>>&& subast) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_b, ::std::move(subast)} {
    }

    constexpr B(::pltxt2htm::B const&) noexcept = default;

    constexpr B(::pltxt2htm::B&& other) noexcept
        : ::pltxt2htm::details::PairedTagBase{::pltxt2htm::NodeType::pl_b, ::std::move(other.subast_)} {
    }

    constexpr ~B() noexcept = default;

    constexpr ::pltxt2htm::B& operator=(::pltxt2htm::B const&) noexcept = delete;

    constexpr ::pltxt2htm::B& operator=(::pltxt2htm::B&&) noexcept = delete;
};

} // namespace pltxt2htm
