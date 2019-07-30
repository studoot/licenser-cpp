#ifndef LICENSE_FORMATTERS_HPP
#define LICENSE_FORMATTERS_HPP

#include "license.hpp"

#include <fmt/core.h>

namespace fmt
{
template <class T>
struct formatter<std::vector<T>>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::vector<T> &t, FormatContext &ctx)
    {
        format_to(ctx.out(), "[ ");
        for (const auto &i : t)
            format_to(ctx.out(), "{}{}", i, (&i == &t.back() ? "" : ", "));
        return format_to(ctx.out(), " ]");
    }
};
} // namespace fmt

namespace fmt
{
template <>
struct formatter<expiry_t>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const expiry_t &t, FormatContext &ctx)
    {
        return std::visit(
            overloaded{[&](const perpetual_t &) {
                           return format_to(ctx.out(), "Expiry{{Perpetual}}");
                       },
                       [&](const term_length_t &t) {
                           using namespace std::literals;
                           static const auto unit_names =
                               std::unordered_map<term_length_t::units_t, std::string>{
                                   {term_length_t::day, "day"s},
                                   {term_length_t::week, "week"s},
                                   {term_length_t::month, "month"s},
                                   {term_length_t::year, "year"s}};
                           return format_to(ctx.out(), "Expiry{{{} {}{}}}", t.count,
                                            unit_names.at(t.units),
                                            t.count != 1 ? "s" : "");
                       },
                       [&](const date::year_month_day &ymd) {
                           return format_to(ctx.out(), "Expiry{{{}}}", ymd);
                       }},
            t);
    }
};
template <>
struct formatter<location_t>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const location_t &t, FormatContext &ctx)
    {
        return std::visit(
            overloaded{[&](const anywhere_t &) {
                           return format_to(ctx.out(), "Location{{Anywhere}}");
                       },
                       [&](const node_t &t) {
                           return format_to(ctx.out(), "Location{{Node {}}}",
                                            t.get());
                       }},
            t);
    }
};
template <>
struct formatter<identity_t>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const identity_t &t, FormatContext &ctx)
    {
        return std::visit(
            overloaded{
                [&](const anyone_t &) {
                    return format_to(ctx.out(), "Identity{{Anyone}}");
                },
                [&](const user_t &t) {
                    return format_to(ctx.out(), "Identity{{User {}}}", t.get());
                },
                [&](const domain_t &t) {
                    return format_to(ctx.out(), "Identity{{Domain {}}}", t.get());
                }},
            t);
    }
};
} // namespace fmt

#endif /* LICENSE_FORMATTERS_HPP */
