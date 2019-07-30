#include "license.hpp"

date::year_month_day term_length_t::get_term_end(const date::year_month_day &start) const
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

auto to_date(const date::year_month_day &eval_date, const date::year_month_day &t)
{
    return t < eval_date ? to_date(eval_date, "Dummy") : t;
}

auto to_date(const date::year_month_day &eval_date, const term_length_t &t)
{
    return t.get_term_end(eval_date);
}

expiry_t get_earliest_expiry(const date::year_month_day &eval_date, const expiry_t &l, const expiry_t &r)
{
    return std::visit(
        [&](const auto &l, const auto &r) {
            return (to_date(eval_date, l) <= to_date(eval_date, r)) ? expiry_t{l} : expiry_t{r};
        },
        l, r);
}

void license_t::process_term(const date::year_month_day &eval_date, const license_term_t &term)
{
    std::visit(
        overloaded{
            [&](secret_t const &s) { secret = s.get(); },
            [&](expiry_t const &e) { expiry = get_earliest_expiry(eval_date, e, expiry); },
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
