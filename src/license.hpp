#ifndef LICENSE_HPP
#define LICENSE_HPP

#include "overloaded.hpp"

#include <chrono>
#include <variant>
#include <vector>

#include <date/date.h>
#include <named_type.hpp>
#include <peglib.h>


struct perpetual_t
{
};

struct term_length_t
{
    enum units_t
    {
        day = 0,
        week,
        month,
        year
    };
    uint16_t count = 0;
    units_t units = day;
    date::year_month_day get_term_end(const date::year_month_day &start) const
    {
        switch (units)
        {
        default:
        case day:
            return date::year_month_day{date::sys_days{start} + date::days{count}};
        case week:
            return date::year_month_day{date::sys_days{start} +
                                        date::days{count * 7}};
        case month:
            return start + date::months{count};
        case year:
            return start + date::years{count};
        };
    }
};
using expiry_t = std::variant<date::year_month_day, term_length_t, perpetual_t>;

template <class T>
auto to_date(const T &t)
{
    return date::year_month_day{date::year::max(), date::month{12},
                                date::day{31}};
}

auto to_date(const date::year_month_day &t)
{
    const auto today = date::year_month_day{
        date::floor<date::days>(std::chrono::system_clock::now())};
    return t < today ? to_date("Dummy") : t;
}

auto to_date(const term_length_t &t)
{
    const auto today = date::year_month_day{
        date::floor<date::days>(std::chrono::system_clock::now())};
    return t.get_term_end(today);
}

expiry_t get_earliest_expiry(const expiry_t &l, const expiry_t &r)
{
    return std::visit(
        [&](const auto &l, const auto &r) {
            return (to_date(l) <= to_date(r)) ? expiry_t{l} : expiry_t{r};
        },
        l, r);
}

using secret_t =
    fluent::NamedType<std::string, struct secret_tag, fluent::Comparable>;

struct anywhere_t
{
};
using node_t =
    fluent::NamedType<std::string, struct node_tag, fluent::Comparable>;
using location_t = std::variant<anywhere_t, node_t>;

struct anyone_t
{
};
using user_t =
    fluent::NamedType<std::string, struct user_tag, fluent::Comparable>;
using domain_t =
    fluent::NamedType<std::string, struct domain_tag, fluent::Comparable>;
using identity_t = std::variant<anyone_t, user_t, domain_t>;

using license_term_t = std::variant<secret_t, expiry_t, location_t, identity_t>;
struct license_t
{
    std::string secret;
    std::vector<license_term_t> terms;
    expiry_t expiry = perpetual_t{};
    std::vector<identity_t> allowed_users;
    std::vector<location_t> allowed_places;

    void process_term(const license_term_t &term)
    {
        std::visit(
            overloaded{
                [&](secret_t const &s) { secret = s.get(); },
                [&](expiry_t const &e) { expiry = get_earliest_expiry(e, expiry); },
                [&](location_t const &loc) {
                    if (allowed_places.size() == 1 &&
                        std::holds_alternative<anywhere_t>(allowed_places.front()))
                    {
                        allowed_places.clear();
                    }
                    if (!std::holds_alternative<anywhere_t>(loc) ||
                        allowed_places.empty())
                    {
                        allowed_places.push_back(loc);
                    }
                },
                [&](identity_t const &id) {
                    if (allowed_users.size() == 1 &&
                        std::holds_alternative<anyone_t>(allowed_users.front()))
                    {
                        allowed_users.clear();
                    }
                    if (!std::holds_alternative<anyone_t>(id) ||
                        allowed_users.empty())
                    {
                        allowed_users.push_back(id);
                    }
                }},
            term);
    }
};

#endif /* LICENSE_HPP */
