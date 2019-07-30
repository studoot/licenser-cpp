#ifndef LICENSE_HPP
#define LICENSE_HPP

#include "overloaded.hpp"

#include <chrono>
#include <variant>
#include <vector>

#include <date/date.h>
#include <named_type.hpp>

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
    date::year_month_day get_term_end(const date::year_month_day &start) const;
};
using expiry_t = std::variant<date::year_month_day, term_length_t, perpetual_t>;

template <class T>
auto to_date(const date::year_month_day & /* eval_date */, const T &t)
{
    return date::year_month_day{date::year::max(), date::month{12},
                                date::day{31}};
}

auto to_date(const date::year_month_day &eval_date, const date::year_month_day &t);

auto to_date(const date::year_month_day &eval_date, const term_length_t &t);

expiry_t get_earliest_expiry(const date::year_month_day &eval_date, const expiry_t &l, const expiry_t &r);

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

    void process_term(const date::year_month_day &eval_date, const license_term_t &term);
};

#endif /* LICENSE_HPP */
