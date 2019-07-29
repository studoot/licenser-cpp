#include <array>
#include <chrono>
#include <fstream>
#include <optional>
#include <sstream>
#include <typeinfo>
#include <variant>

#include <date/date.h>
#define FMT_STRING_ALIAS 1
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <peglib.h>
using namespace peg;

#include "license.peg.h"

namespace fmt
{
template <class T>
struct formatter<std::optional<T>>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::optional<T> &t, FormatContext &ctx)
    {
        if (t)
            return formatter<T>::format(*t, ctx);
        else
            return format_to(ctx.out(), "None");
    }
};
} // namespace fmt

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

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
            return date::year_month_day{date::sys_days{start} + date::days{count * 7}};
        case month:
            return start + date::months{count};
        case year:
            return start + date::years{count};
        };
    }
};
using expiry_t = std::variant<date::year_month_day, term_length_t, perpetual_t>;

namespace fmt
{
template <>
struct formatter<expiry_t>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const expiry_t &t, FormatContext &ctx)
    {
        return std::visit(overloaded{
                              [&](const perpetual_t &) {
                                  return format_to(ctx.out(), "Expiry{{Perpetual}}");
                              },
                              [&](const term_length_t &t) {
                                  using namespace std::literals;
                                  static constexpr std::array<const char *, 4> unit_names = {"day", "week", "month", "year"};
                                  return format_to(ctx.out(), "Expiry{{{} {}{}}}", t.count, unit_names[t.units], t.count != 1 ? "s" : "");
                              },
                              [&](const date::year_month_day &ymd) {
                                  return format_to(ctx.out(), "Expiry{{{}}}", ymd);
                              }},
                          t);
    }
};
} // namespace fmt

template <class T>
auto to_date(const T &t)
{
    return date::year_month_day{date::year::max(), date::month{12}, date::day{31}};
}

auto to_date(const date::year_month_day &t)
{
    const auto today = date::year_month_day{date::floor<date::days>(std::chrono::system_clock::now())};
    return t < today ? to_date("Dummy") : t;
}

auto to_date(const term_length_t &t)
{
    const auto today = date::year_month_day{date::floor<date::days>(std::chrono::system_clock::now())};
    return t.get_term_end(today);
}

expiry_t get_earliest_expiry(const expiry_t &l, const expiry_t &r)
{
    return std::visit(
        [&](const auto &l, const auto &r) {
            fmt::print("get_earliest_expiry({}, {}) -> {}\n", to_date(l),to_date(r), (to_date(l) <= to_date(r)) ? expiry_t{l} : expiry_t{r});
            return (to_date(l) <= to_date(r)) ? expiry_t{l} : expiry_t{r}; },
        l, r);
}

struct secret_t
{
    std::string secret;
};

using license_term_t = std::variant<secret_t, expiry_t>;
static_assert(std::variant_size_v<license_term_t> == 2);
struct license_t
{
    std::string secret;
    std::vector<license_term_t> terms;
    expiry_t expiry = perpetual_t{};
    // std::vector<identity_term_t> allowed_users;
    // std::vector<location_term_t> allowed_places;
};

uint16_t to_natural(const SemanticValues &sv) { return std::stoul(sv.str()); }

term_length_t::units_t to_term_unit(const SemanticValues &sv)
{
    return static_cast<term_length_t::units_t>(sv.choice());
}

uint16_t from_month_name(const SemanticValues &sv)
{
    return static_cast<uint16_t>(sv.choice() + 1);
}

std::string
read_file(std::string const &filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        std::ostringstream contents;
        contents << in.rdbuf();
        in.close();
        return (contents.str());
    }
    return std::string{};
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        parser p;
        p.log = [](size_t line, size_t col, const std::string &msg) {
            fmt::print(std::clog, "{}:{}: {}\n", line, col, msg);
        };

        if (p.load_grammar(reinterpret_cast<const char *>(license_peg)))
        {
            p["NATURAL"] = to_natural;

            p["License"] = [](const SemanticValues &sv) {
                license_t license;
                license.terms = sv.transform<license_term_t>();
                for (const auto &term : license.terms)
                {
                    switch (term.index())
                    {
                    case 0:
                        license.secret = std::get<0>(term).secret;
                        break;
                    case 1:
                        license.expiry = get_earliest_expiry(std::get<1>(term), license.expiry);
                        break;
                    default:
                        break;
                    };
                }
                return license;
            };

            p["SecretTerm"] = [](const SemanticValues &sv) {
                return license_term_t{secret_t{sv.token(0)}};
            };

            p["TimeTerm"] = [](const SemanticValues &sv) {
                return license_term_t(sv[0].get<expiry_t>());
            };

            p["TermUnit"] = to_term_unit;
            p["TermLength"] = [](const SemanticValues &sv) {
                return expiry_t{term_length_t{sv[0].get<uint16_t>(),
                                              sv[1].get<term_length_t::units_t>()}};
            };

            p["ISOYEAR"] = to_natural;
            p["ISOMONTH"] = to_natural;
            p["ISODAY"] = to_natural;
            p["ISO8601"] = [](const SemanticValues &sv) {
                return expiry_t{date::year_month_day{date::year{sv[0].get<uint16_t>()},
                                                     date::month{sv[1].get<uint16_t>()},
                                                     date::day{sv[2].get<uint16_t>()}}};
            };

            p["YEAR"] = to_natural;
            p["MonthName"] = from_month_name;
            p["DAY"] = to_natural;
            p["NamedDate"] = [](const SemanticValues &sv) {
                return expiry_t{date::year_month_day{date::year{sv[2].get<uint16_t>()},
                                                     date::month{sv[1].get<uint16_t>()},
                                                     date::day{sv[0].get<uint16_t>()}}};
            };

            p["PerpetualTerm"] = [](const SemanticValues &sv) {
                return expiry_t{perpetual_t{}};
            };
            p.enable_packrat_parsing();
#if 0
            size_t prev_pos = 0;
            p.enable_trace([&](
                               const char *name,
                               const char *s,
                               size_t /*n*/,
                               const peg::SemanticValues & /*sv*/,
                               const peg::Context &c,
                               const peg::any & /*dt*/) {
                auto pos = static_cast<size_t>(s - c.s);
                auto backtrack = (pos < prev_pos ? "*" : "");
                fmt::print(fmt("{}:{}:{}\t{:>{}}\n"), pos, c.nest_level, backtrack, name, c.nest_level * 2);
                prev_pos = static_cast<size_t>(pos);
            });
#endif
            const auto file = read_file(argv[1]);
            license_t license;
            p.parse(file.c_str(), license, argv[1]);
            fmt::print("License secret = {}, expiry = {}\n", license.secret, license.expiry);
        }
    }
}
