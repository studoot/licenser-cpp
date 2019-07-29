#include "license.hpp"

#include <array>
#include <fstream>
#include <sstream>

#define FMT_STRING_ALIAS 1
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
using namespace peg;

#include "license.peg.h"
#include "overloaded.hpp"

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

uint16_t to_natural(const SemanticValues &sv) { return std::stoul(sv.str()); }

term_length_t::units_t to_term_unit(const SemanticValues &sv)
{
    return static_cast<term_length_t::units_t>(sv.choice());
}

uint16_t from_month_name(const SemanticValues &sv)
{
    return static_cast<uint16_t>(sv.choice() + 1);
}

std::string read_file(std::string const &filename)
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
                    license.process_term(term);
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

            p["LocationTerm"] = [](const SemanticValues &sv) {
                switch (sv.choice())
                {
                default:
                case 0:
                    return license_term_t{location_t{anywhere_t{}}};
                case 1:
                    return license_term_t{sv[0].get<location_t>()};
                }
            };
            p["NodeTerm"] = [](const SemanticValues &sv) {
                return location_t{node_t{sv[0].get<std::string>()}};
            };

            p["IdentityTerm"] = [](const SemanticValues &sv) {
                switch (sv.choice())
                {
                default:
                case 0:
                    return license_term_t{identity_t{anyone_t{}}};
                case 1:
                case 2:
                    return license_term_t{sv[0].get<identity_t>()};
                }
            };
            p["UserTerm"] = [](const SemanticValues &sv) {
                return identity_t{user_t{sv[0].get<std::string>()}};
            };
            p["DomainTerm"] = [](const SemanticValues &sv) {
                return identity_t{domain_t{sv[0].get<std::string>()}};
            };
            p["NO_SPACE_STRING"] = [](const SemanticValues &sv) { return sv.str(); };
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
            fmt::print("License secret = {}, expiry = {}, locn = {}, id = {}\n",
                       license.secret, license.expiry, license.allowed_places,
                       license.allowed_users);
        }
    }
}
