#pragma once

#include <algorithm>
#include <exception/exception.hh>
#include <fast_io/fast_io_dsal/vector.h>
#include <fast_io/fast_io_dsal/string.h>
#include <fast_io/fast_io_dsal/string_view.h>
#include "../astnode/node_type.hh"
#include "../astnode/basic.hh"
#include "../astnode/html_node.hh"
#include "../astnode/physics_lab_node.hh"
#include "../heap_guard.hh"

namespace pltxt2htm::details {

constexpr ::fast_io::u8string size_t2str(::std::size_t num) noexcept {
    if (num == 0) {
        return ::fast_io::u8string{u8"0"};
    }

    ::fast_io::u8string result;

    while (num > 0) {
        char8_t digit = (num % 10) + u8'0';
        result.push_back(digit);
        num /= 10;
    }

    ::std::reverse(result.begin(), result.end());

    return result;
}

/**
 * @brief Integrate ast nodes to HTML.
 * @tparam ndebug: true  -> release mode, disables most of the checks which is unsafe but fast
 *                 false -> debug mode, enables most of the checks
 * @param [in] ast: Ast of Quantum-Physics's text
 * @param [in] extern_node: The pointer of parent node
 */
template<bool ndebug>
[[nodiscard]]
constexpr auto ast2advanced_html(::fast_io::vector<::pltxt2htm::details::HeapGuard<::pltxt2htm::PlTxtNode>> const& ast,
                                 ::fast_io::u8string_view host,
                                 ::pltxt2htm::PlTxtNode const* const extern_node = nullptr)
#if __cpp_exceptions < 199711L
    noexcept
#endif
    -> ::fast_io::u8string {
    ::fast_io::u8string result{};

    for (auto&& node : ast) {
        switch (node->node_type()) {
        case ::pltxt2htm::NodeType::u8char: {
            result.push_back(reinterpret_cast<::pltxt2htm::U8Char const*>(node.release_imul())->get_u8char());
            break;
        }
        case ::pltxt2htm::NodeType::space: {
            result.append(u8"&nbsp;");
            break;
        }
        case ::pltxt2htm::NodeType::ampersand: {
            result.append(u8"&amp;");
            break;
        }
        case ::pltxt2htm::NodeType::single_quote: {
            result.append(u8"&apos;");
            break;
        }
        case ::pltxt2htm::NodeType::double_quote: {
            result.append(u8"&quot;");
            break;
        }
        case ::pltxt2htm::NodeType::less_than: {
            result.append(u8"&lt;");
            break;
        }
        case ::pltxt2htm::NodeType::greater_than: {
            result.append(u8"&gt;");
            break;
        }
        case ::pltxt2htm::NodeType::pl_color:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::pl_a: {
            // <a> and <color> is the same tag&struct in fact
            auto color = reinterpret_cast<::pltxt2htm::Color const*>(node.release_imul());
            auto&& subast = color->get_subast();
            if (subast.size() == 1) {
                // <color=red><color=blue>text</color></color> can be optimized
                auto subnode = subast.index_unchecked(0).release_imul();
                if (subnode->node_type() == ::pltxt2htm::NodeType::pl_color) {
                    color = reinterpret_cast<::pltxt2htm::Color const*>(subnode);
                }
            }
            // Optimization: If the color is the same as the parent node, then ignore the nested tag.
            bool const is_not_same_tag =
                extern_node == nullptr ||
                (extern_node->node_type() != ::pltxt2htm::NodeType::pl_color &&
                 extern_node->node_type() != ::pltxt2htm::NodeType::pl_a) ||
                color->get_color() != reinterpret_cast<::pltxt2htm::Color const*>(extern_node)->get_color();
            if (is_not_same_tag) {
                result.append(u8"<span style=\"color:");
                result.append(color->get_color());
                result.append(u8";\">");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(subast, host, color));
            if (is_not_same_tag) {
                result.append(u8"</span>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::pl_experiment: {
            auto experiment = reinterpret_cast<::pltxt2htm::Experiment const*>(node.release_imul());
            auto&& subast = experiment->get_subast();
            if (subast.size() == 1) {
                // <Experiment=123><experiment=642cf37a494746375aae306a>physicsLab</experiment></Experiment> can be
                // optimized as <a href=\"localhost:5173/ExperimentSummary/Experiment/642cf37a494746375aae306a\"
                auto subnode = subast.index_unchecked(0).release_imul();
                if (subnode->node_type() == ::pltxt2htm::NodeType::pl_experiment) {
                    experiment = reinterpret_cast<::pltxt2htm::Experiment const*>(subnode);
                }
            }
            // Optimization: If the experiment is the same as the parent node, then ignore the nested tag.
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::pl_experiment ||
                experiment->get_id() != reinterpret_cast<::pltxt2htm::Experiment const*>(extern_node)->get_id();
            // internal>physicsLab</a>
            if (is_not_same_tag) {
                result.append(u8"<a href=\"");
                result.append(host);
                result.append(u8"/ExperimentSummary/Experiment/");
                result.append(experiment->get_id());
                result.append(u8"\" internal>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(subast, host, experiment));
            if (is_not_same_tag) {
                result.append(u8"</a>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::pl_discussion: {
            auto discussion = reinterpret_cast<::pltxt2htm::Discussion const*>(node.release_imul());
            auto&& subast = discussion->get_subast();
            if (subast.size() == 1) {
                // <Discussion=123><discussion=642cf37a494746375aae306a>physicsLab</discussion></Discussion> can be
                // optimized as <a href=\"localhost:5173/ExperimentSummary/Discussion/642cf37a494746375aae306a\"
                // internal>physicsLab</a>
                auto subnode = subast.index_unchecked(0).release_imul();
                if (subnode->node_type() == ::pltxt2htm::NodeType::pl_discussion) {
                    discussion = reinterpret_cast<::pltxt2htm::Discussion const*>(subnode);
                }
            }
            // Optimization: If the discussion is the same as the parent node, then ignore the nested tag.
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::pl_discussion ||
                discussion->get_id() != reinterpret_cast<::pltxt2htm::Discussion const*>(extern_node)->get_id();
            if (is_not_same_tag) {
                result.append(u8"<a href=\"");
                result.append(host);
                result.append(u8"/ExperimentSummary/Discussion/");
                result.append(discussion->get_id());
                result.append(u8"\" internal>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(discussion->get_subast(), host, discussion));
            if (is_not_same_tag) {
                result.append(u8"</a>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::pl_user: {
            auto user = reinterpret_cast<::pltxt2htm::User const*>(node.release_imul());
            auto&& subast = user->get_subast();
            if (subast.size() == 1) {
                // <User=123><user=642cf37a494746375aae306a>physicsLab</user></User> can be
                auto subnode = subast.index_unchecked(0).release_imul();
                if (subnode->node_type() == ::pltxt2htm::NodeType::pl_user) {
                    user = reinterpret_cast<::pltxt2htm::User const*>(subnode);
                }
            }
            // Optimization: If the user is the same as the parent node, then ignore the nested tag.
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::pl_user ||
                user->get_id() != reinterpret_cast<::pltxt2htm::User const*>(extern_node)->get_id();
            if (is_not_same_tag) {
                result.append(u8"<span class=\'RUser\' data-user=\'");
                result.append(user->get_id());
                result.append(u8"\'>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(user->get_subast(), host, user));
            if (is_not_same_tag) {
                result.append(u8"</span>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::pl_size: {
            auto size = reinterpret_cast<::pltxt2htm::Size const*>(node.release_imul());
            auto&& subast = size->get_subast();
            if (subast.size() == 1) {
                // <size=123><size=642cf37a494746375aae306a>physicsLab</size></size> can be
                auto subnode = subast.index_unchecked(0).release_imul();
                if (subnode->node_type() == ::pltxt2htm::NodeType::pl_size) {
                    size = reinterpret_cast<::pltxt2htm::Size const*>(subnode);
                }
            }
            // Optimization: If the size is the same as the parent node, then ignore the nested tag.
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::pl_size ||
                size->get_id() != reinterpret_cast<::pltxt2htm::Size const*>(extern_node)->get_id();
            if (is_not_same_tag) {
                result.append(u8"<span style=\"font-size:");
                result.append(::pltxt2htm::details::size_t2str(size->get_id() / 2));
                result.append(u8"px\">");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(size->get_subast(), host, size));
            if (is_not_same_tag) {
                result.append(u8"</span>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::pl_b: {
            auto b = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::pl_b;
            if (is_not_same_tag) {
                result.append(u8"<b>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(b->get_subast(), host, b));
            if (is_not_same_tag) {
                result.append(u8"</b>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::pl_i: {
            auto i = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::pl_i;
            if (is_not_same_tag) {
                result.append(u8"<i>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(i->get_subast(), host, i));
            if (is_not_same_tag) {
                result.append(u8"</i>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_p: {
            auto p = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_p;
            if (is_not_same_tag) {
                result.append(u8"<p>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(p->get_subast(), host, p));
            if (is_not_same_tag) {
                result.append(u8"</p>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::line_break:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::html_br: {
            result.append(u8"<br>");
            break;
        }
        case ::pltxt2htm::NodeType::html_h1:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::md_atx_h1: {
            auto h1 = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_h1;
            if (is_not_same_tag) {
                result.append(u8"<h1>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(h1->get_subast(), host, h1));
            if (is_not_same_tag) {
                result.append(u8"</h1>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_h2:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::md_atx_h2: {
            auto h2 = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_h2;
            if (is_not_same_tag) {
                result.append(u8"<h2>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(h2->get_subast(), host, h2));
            if (is_not_same_tag) {
                result.append(u8"</h2>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_h3:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::md_atx_h3: {
            auto h3 = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_h3;
            if (is_not_same_tag) {
                result.append(u8"<h3>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(h3->get_subast(), host, h3));
            if (is_not_same_tag) {
                result.append(u8"</h3>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_h4:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::md_atx_h4: {
            auto h4 = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_h4;
            if (is_not_same_tag) {
                result.append(u8"<h4>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(h4->get_subast(), host, h4));
            if (is_not_same_tag) {
                result.append(u8"</h4>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_h5:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::md_atx_h5: {
            auto h5 = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_h5;
            if (is_not_same_tag) {
                result.append(u8"<h5>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(h5->get_subast(), host, h5));
            if (is_not_same_tag) {
                result.append(u8"</h5>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_h6:
            [[fallthrough]];
        case ::pltxt2htm::NodeType::md_atx_h6: {
            auto h6 = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_h6;
            if (is_not_same_tag) {
                result.append(u8"<h6>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(h6->get_subast(), host, h6));
            if (is_not_same_tag) {
                result.append(u8"</h6>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::html_del: {
            auto del = reinterpret_cast<::pltxt2htm::details::PairedTagBase const*>(node.release_imul());
            bool const is_not_same_tag =
                extern_node == nullptr || extern_node->node_type() != ::pltxt2htm::NodeType::html_del;
            if (is_not_same_tag) {
                result.append(u8"<del>");
            }
            result.append(::pltxt2htm::details::ast2advanced_html<ndebug>(del->get_subast(), host, del));
            if (is_not_same_tag) {
                result.append(u8"</del>");
            }
            break;
        }
        case ::pltxt2htm::NodeType::base:
#if 0
     [[unlikely]] [[fallthrough]];
     default:
#endif
            [[unlikely]] {
                ::exception::unreachable<ndebug>();
            }
        }
    }

    return result;
}

} // namespace pltxt2htm::details
