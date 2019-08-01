#include "license.hpp"

#include <doctest/doctest.h>

date::year_month_day term_length_t::get_term_end(const date::year_month_day &start) const
{
    switch (units)
    {
    default:
    case day:
        return date::year_month_day{date::sys_days{start} + date::days{count}};
    case week:
        return date::year_month_day{date::sys_days{start} + date::days{count * 7}};
    case month:
    {
        auto term_end = start + date::months{count};
        if (!term_end.ok()) { term_end = term_end.year() / term_end.month() / date::last; }
        return term_end;
    }
    case year:
        return start + date::years{count};
    };
}

TEST_CASE("test to_date for term length")
{
    using namespace date;
    {
        const auto now = 2019_y / 07 / 30;
        REQUIRE(term_length_t{2, term_length_t::day}.get_term_end(now) == 2019_y / 8 / 1);
        REQUIRE(term_length_t{4, term_length_t::week}.get_term_end(now) == 2019_y / 8 / 27);
        REQUIRE(term_length_t{3, term_length_t::month}.get_term_end(now) == 2019_y / 10 / 30);
        REQUIRE(term_length_t{7, term_length_t::month}.get_term_end(now) == 2020_y / 2 / 29);
        REQUIRE(term_length_t{19, term_length_t::month}.get_term_end(now) == 2021_y / 2 / 28);
        REQUIRE(term_length_t{2, term_length_t::year}.get_term_end(now) == 2021_y / 7 / 30);
    }
    {
        const auto now = 2019_y / 02 / 28;
        REQUIRE(term_length_t{1, term_length_t::month}.get_term_end(now) == 2019_y / 3 / 28);
    }
}

auto to_date(const date::year_month_day &eval_date, const date::year_month_day &t)
{
    return t < eval_date ? to_date(eval_date, "Dummy") : t;
}

TEST_CASE("test to_date for fixed date")
{
    using namespace date;
    const auto now = 2019_y / 07 / 30;
    REQUIRE(to_date(now, 2020_y / 1 / 1) == 2020_y / 1 / 1);
    REQUIRE(to_date(now, 2019_y / 1 / 1) == year::max() / 12 / 31);
}

auto to_date(const date::year_month_day &eval_date, const term_length_t &t)
{
    return t.get_term_end(eval_date);
}

TEST_CASE("test to_date for term length")
{
    using namespace date;
    {
        const auto now = 2019_y / 07 / 30;
        REQUIRE(to_date(now, term_length_t{2, term_length_t::day}) == 2019_y / 8 / 1);
        REQUIRE(to_date(now, term_length_t{4, term_length_t::week}) == 2019_y / 8 / 27);
        REQUIRE(to_date(now, term_length_t{3, term_length_t::month}) == 2019_y / 10 / 30);
        REQUIRE(to_date(now, term_length_t{7, term_length_t::month}) == 2020_y / 2 / 29);
        REQUIRE(to_date(now, term_length_t{19, term_length_t::month}) == 2021_y / 2 / 28);
        REQUIRE(to_date(now, term_length_t{2, term_length_t::year}) == 2021_y / 7 / 30);
    }
    {
        const auto now = 2019_y / 02 / 28;
        REQUIRE(to_date(now, term_length_t{1, term_length_t::month}) == 2019_y / 3 / 28);
    }
}

expiry_t get_earliest_expiry(const date::year_month_day &eval_date, const expiry_t &l, const expiry_t &r)
{
    return std::visit([&](const auto &l,
                          const auto &
                              r) { return (to_date(eval_date, l) <= to_date(eval_date, r)) ? expiry_t{l} : expiry_t{r}; },
                      l, r);
}

TEST_CASE("test expiry comparisons")
{
    using namespace date;
    const auto now = 2019_y / 07 / 30;
    REQUIRE(get_earliest_expiry(now, term_length_t{2, term_length_t::day}, perpetual_t{}) ==
            expiry_t{term_length_t{2, term_length_t::day}});
    REQUIRE(get_earliest_expiry(now, 2020_y / 2 / 12, perpetual_t{}) == expiry_t{2020_y / 2 / 12});
    REQUIRE(get_earliest_expiry(now, perpetual_t{}, term_length_t{2, term_length_t::day}) ==
            expiry_t{term_length_t{2, term_length_t::day}});
    REQUIRE(get_earliest_expiry(now, perpetual_t{}, 2020_y / 2 / 12) == expiry_t{2020_y / 2 / 12});
    REQUIRE(get_earliest_expiry(now, 2020_y / 2 / 12, term_length_t{2, term_length_t::day}) ==
            expiry_t{term_length_t{2, term_length_t::day}});
    REQUIRE(get_earliest_expiry(now, term_length_t{2, term_length_t::day}, 2020_y / 2 / 12) ==
            expiry_t{term_length_t{2, term_length_t::day}});
    REQUIRE(get_earliest_expiry(now, 2020_y / 2 / 12, term_length_t{12, term_length_t::month}) ==
            expiry_t{2020_y / 2 / 12});
    REQUIRE(get_earliest_expiry(now, term_length_t{12, term_length_t::month}, 2020_y / 2 / 12) ==
            expiry_t{2020_y / 2 / 12});
}

void license_t::process_term(const date::year_month_day &eval_date, const license_term_t &term)
{
    std::visit(overloaded{[&](secret_t const &s) { secret = s.get(); },
                          [&](expiry_t const &e) { expiry = get_earliest_expiry(eval_date, e, expiry); },
                          [&](location_t const &loc) {
                              if (allowed_places.size() == 1 &&
                                  std::holds_alternative<anywhere_t>(allowed_places.front()))
                              { allowed_places.clear(); }
                              if (!std::holds_alternative<anywhere_t>(loc) || allowed_places.empty())
                              { allowed_places.push_back(loc); }
                          },
                          [&](identity_t const &id) {
                              if (allowed_users.size() == 1 && std::holds_alternative<anyone_t>(allowed_users.front()))
                              { allowed_users.clear(); }
                              if (!std::holds_alternative<anyone_t>(id) || allowed_users.empty())
                              { allowed_users.push_back(id); }
                          }},
               term);
}

TEST_CASE("test term processing")
{
    using namespace date;
    const auto now = 2019_y / 07 / 30;
    license_t l;

    SUBCASE("Add secret")
    {
        REQUIRE(l.secret.empty());
        l.process_term(now, secret_t{"Hello world"});
        REQUIRE(l.secret == "Hello world");
        l.process_term(now, secret_t{"Another secret"});
        REQUIRE(l.secret == "Another secret");
    }
    SUBCASE("Add location, then ANYWHERE")
    {
        REQUIRE(l.allowed_places.empty());
        l.process_term(now, location_t{node_t{"a-node"}});
        REQUIRE(l.allowed_places.size() == 1);
        l.process_term(now, location_t{anywhere_t{}});
        REQUIRE(l.allowed_places.size() == 1);
        REQUIRE(std::holds_alternative<node_t>(l.allowed_places.front()));
        l.process_term(now, location_t{node_t{"b-node"}});
        REQUIRE(l.allowed_places.size() == 2);
    }
    SUBCASE("Add ANYWHERE, then valid locations")
    {
        REQUIRE(l.allowed_places.empty());
        l.process_term(now, location_t{anywhere_t{}});
        REQUIRE(l.allowed_places.size() == 1);
        REQUIRE(std::holds_alternative<anywhere_t>(l.allowed_places.front()));
        l.process_term(now, location_t{node_t{"a-node"}});
        REQUIRE(l.allowed_places.size() == 1);
        REQUIRE(std::holds_alternative<node_t>(l.allowed_places.front()));
        l.process_term(now, location_t{node_t{"b-node"}});
        REQUIRE(l.allowed_places.size() == 2);
    }
    SUBCASE("Add identity, then ANYONE")
    {
        SUBCASE("User first")
        {
            REQUIRE(l.allowed_users.empty());
            l.process_term(now, identity_t{user_t{"a-person"}});
            REQUIRE(l.allowed_users.size() == 1);
            l.process_term(now, identity_t{anyone_t{}});
            REQUIRE(l.allowed_users.size() == 1);
            REQUIRE(std::holds_alternative<user_t>(l.allowed_users.front()));
            l.process_term(now, identity_t{user_t{"another-person"}});
            REQUIRE(l.allowed_users.size() == 2);
            l.process_term(now, identity_t{domain_t{"a-domain"}});
            REQUIRE(l.allowed_users.size() == 3);
        }
        SUBCASE("Domain first")
        {
            REQUIRE(l.allowed_users.empty());
            l.process_term(now, identity_t{domain_t{"a-domain"}});
            REQUIRE(l.allowed_users.size() == 1);
            l.process_term(now, identity_t{anyone_t{}});
            REQUIRE(l.allowed_users.size() == 1);
            REQUIRE(std::holds_alternative<domain_t>(l.allowed_users.front()));
            l.process_term(now, identity_t{user_t{"another-person"}});
            REQUIRE(l.allowed_users.size() == 2);
            l.process_term(now, identity_t{domain_t{"another-domain"}});
            REQUIRE(l.allowed_users.size() == 3);
        }
    }
    SUBCASE("Add ANYONE, then valid identities")
    {
        REQUIRE(l.allowed_users.empty());
        l.process_term(now, identity_t{anyone_t{}});
        REQUIRE(l.allowed_users.size() == 1);
        REQUIRE(std::holds_alternative<anyone_t>(l.allowed_users.front()));
        SUBCASE("Add a user first")
        {
            l.process_term(now, identity_t{user_t{"a-user"}});
            REQUIRE(l.allowed_users.size() == 1);
            REQUIRE(std::holds_alternative<user_t>(l.allowed_users.front()));
            l.process_term(now, identity_t{user_t{"another-user"}});
            REQUIRE(l.allowed_users.size() == 2);
            l.process_term(now, identity_t{domain_t{"a-domain"}});
            REQUIRE(l.allowed_users.size() == 3);
        }
        SUBCASE("Add a domain first")
        {
            l.process_term(now, identity_t{domain_t{"a-domain"}});
            REQUIRE(l.allowed_users.size() == 1);
            REQUIRE(std::holds_alternative<domain_t>(l.allowed_users.front()));
            l.process_term(now, identity_t{user_t{"a-user"}});
            REQUIRE(l.allowed_users.size() == 2);
            l.process_term(now, identity_t{domain_t{"another-domain"}});
            REQUIRE(l.allowed_users.size() == 3);
        }
    }
    SUBCASE("Add expiries")
    {
        REQUIRE(std::holds_alternative<perpetual_t>(l.expiry));
        l.process_term(now, expiry_t{perpetual_t{}});
        REQUIRE(std::holds_alternative<perpetual_t>(l.expiry));
        l.process_term(now, expiry_t{2020_y / 2 / 1});
        REQUIRE(l.expiry == expiry_t{2020_y / 2 / 1});
        l.process_term(now, expiry_t{perpetual_t{}});
        REQUIRE(l.expiry == expiry_t{2020_y / 2 / 1});
        l.process_term(now, expiry_t{term_length_t{3, term_length_t::week}});
        REQUIRE(l.expiry == expiry_t{term_length_t{3, term_length_t::week}});
        l.process_term(now, expiry_t{perpetual_t{}});
        REQUIRE(l.expiry == expiry_t{term_length_t{3, term_length_t::week}});
        l.process_term(now, expiry_t{2020_y / 2 / 1});
        REQUIRE(l.expiry == expiry_t{term_length_t{3, term_length_t::week}});
        l.process_term(now, expiry_t{2019_y / 8 / 8});
        REQUIRE(l.expiry == expiry_t{2019_y / 8 / 8});
        l.process_term(now, expiry_t{term_length_t{3, term_length_t::week}});
        REQUIRE(l.expiry == expiry_t{2019_y / 8 / 8});
    }
}
