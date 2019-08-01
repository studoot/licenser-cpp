#include "license-parser.hpp"

#include "license.peg.hpp"
#include "overloaded.hpp"

#include <fmt/core.h>
#include <peglib.h>
using namespace peg;

uint16_t to_natural(const SemanticValues &sv)
{
    return std::stoul(sv.str());
}

term_length_t::units_t to_term_unit(const SemanticValues &sv)
{
    return static_cast<term_length_t::units_t>(sv.choice());
}

expiry_t validate_ymd(uint16_t year, uint16_t month, uint16_t day)
{
    const auto y = date::year{year};
    if (!y.ok()) throw peg::parse_error(fmt::format("bad year {}", year).c_str());
    const auto ym = y / month;
    if (!ym.ok()) throw peg::parse_error(fmt::format("bad month {}", month).c_str());
    const auto ymd = ym / day;
    if (!ymd.ok()) throw peg::parse_error(fmt::format("bad day {}", day).c_str());
    return ymd;
}

expiry_t to_dmy_expiry(const SemanticValues &sv)
{
    return validate_ymd(sv[2].get<uint16_t>(), sv[1].get<uint16_t>(), sv[0].get<uint16_t>());
}

expiry_t to_ymd_expiry(const SemanticValues &sv)
{
    return validate_ymd(sv[0].get<uint16_t>(), sv[1].get<uint16_t>(), sv[2].get<uint16_t>());
}

std::optional<parser> prepare_parser()
{
    parser p;

    if (!p.load_grammar(reinterpret_cast<const char *>(license_peg))) return std::nullopt;
    p["NATURAL"] = to_natural;

    p["License"] = [&](const SemanticValues &sv) {
        license_t license;
        license.terms = sv.transform<license_term_t>();
        return license;
    };

    p["SecretTerm"] = [](const SemanticValues &sv) { return license_term_t{secret_t{sv.token(0)}}; };

    p["TimeTerm"] = [](const SemanticValues &sv) { return license_term_t(sv[0].get<expiry_t>()); };

    p["TermUnit"] = to_term_unit;
    p["TermLength"] = [](const SemanticValues &sv) {
        return expiry_t{term_length_t{sv[0].get<uint16_t>(), sv[1].get<term_length_t::units_t>()}};
    };

    p["ISOYEAR"] = to_natural;
    p["ISOMONTH"] = to_natural;
    p["ISODAY"] = to_natural;
    p["ISO8601"] = to_ymd_expiry;

    p["YEAR"] = to_natural;
    p["MonthName"] = [](const SemanticValues &sv) { return static_cast<uint16_t>(sv.choice() + 1); };
    p["DAY"] = to_natural;
    p["NamedDate"] = to_dmy_expiry;

    p["PerpetualTerm"] = [](const SemanticValues &sv) { return expiry_t{perpetual_t{}}; };

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
    p["NodeTerm"] = [](const SemanticValues &sv) { return location_t{node_t{sv[0].get<std::string>()}}; };

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
    p["UserTerm"] = [](const SemanticValues &sv) { return identity_t{user_t{sv[0].get<std::string>()}}; };
    p["DomainTerm"] = [](const SemanticValues &sv) { return identity_t{domain_t{sv[0].get<std::string>()}}; };
    p["NO_SPACE_STRING"] = [](const SemanticValues &sv) { return sv.str(); };
    return p;
}

std::optional<license_t> parse_license(const date::year_month_day &eval_date,
                                       std::string_view text,
                                       std::optional<std::string> const &from_file)
{
    if (auto parser = prepare_parser())
    {
        license_t license;
        parser->parse_n(text.data(), text.size(), license, from_file.value_or("").c_str());
        for (const auto &term : license.terms)
        {
            license.process_term(eval_date, term);
        }
        return license;
    }
    return std::nullopt;
}

#if !defined(DOCTEST_CONFIG_DISABLE)
#include "license-formatters.hpp"

#include <doctest/doctest.h>
#include <fmt/core.h>

struct Tracer
{
    Tracer()
    {
        std::cout << "pos:lev\trule/ope" << std::endl;
        std::cout << "-------\t--------" << std::endl;
    }
    size_t prev_pos = 0;
    void operator()(const char *name,
                    const char *s,
                    size_t /*n*/,
                    const peg::SemanticValues & /*sv*/,
                    const peg::Context &c,
                    const peg::any & /*dt*/)
    {
        auto pos = static_cast<size_t>(s - c.s);
        auto backtrack = (pos < prev_pos ? "*" : "");
        std::string indent;
        auto level = c.nest_level;
        while (level--)
        {
            indent += "  ";
        }
        std::cout << pos << ":" << c.nest_level << backtrack << "\t" << indent << name << std::endl;
        prev_pos = static_cast<size_t>(pos);
    }
};

namespace peg
{
bool operator==(const Definition::Result &l, const Definition::Result &r)
{
    return l.ret == r.ret && l.len == r.len;
}
doctest::String toString(const Definition::Result &value)
{
    return doctest::String(
        fmt::format("Definition::Result{{ .ret={}, .len={}, .message={} }}", value.ret, value.len, value.message).c_str());
}
} // namespace peg
template <class T>
using res_t = std::variant<T, Definition::Result>;
template <class T>
res_t<T> parse_failure{Definition::Result{false, size_t(-1)}};
namespace std
{
template <class T>
std::ostream &operator<<(std::ostream &os, const res_t<T> &value)
{
    if (value.index() == 0)
        return os << std::get<0>(value);
    else
        return os << fmt::format("Definition::Result{{ .ret={}, .len={}, .message={} }}", std::get<1>(value).ret,
                                 std::get<1>(value).len, std::get<1>(value).message);
}
} // namespace std
template <class T>
res_t<T> test_parse(Definition &d, std::string_view sv)
{
    T val;
    if (const auto r = d.parse_and_get_value(sv.data(), sv.size(), val); !r.ret || r.len < sv.size())
    { return parse_failure<T>; }
    return val;
}

using namespace std::literals;
TEST_CASE("MonthName")
{
    auto p = *prepare_parser();
    REQUIRE(test_parse<uint16_t>(p["MonthName"], "Jan"sv) == res_t<uint16_t>{1});
    REQUIRE(test_parse<uint16_t>(p["MonthName"], "jul"sv) == res_t<uint16_t>{7});
    REQUIRE(test_parse<uint16_t>(p["MonthName"], "OCTOBER"sv) == res_t<uint16_t>{10});
    REQUIRE(test_parse<uint16_t>(p["MonthName"], "Octiber"sv) == parse_failure<uint16_t>);
    REQUIRE(test_parse<uint16_t>(p["MonthName"], "Ocsober"sv) == parse_failure<uint16_t>);
}

TEST_CASE("ISO8601")
{
    using namespace date;
    auto p = *prepare_parser();
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-07-31"sv) == res_t<expiry_t>{expiry_t{2019_y / 7 / 31}});
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-07-32"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-01-00"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-13-01"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-00-01"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "20190-13-01"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-7-01"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["ISO8601"], "2019-12-1"sv) == parse_failure<expiry_t>);
}

TEST_CASE("NamedDate")
{
    using namespace date;
    auto p = *prepare_parser();
    p["NamedDate"].whitespaceOpe = p["License"].whitespaceOpe;
    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "01 Jan 1900"sv) == res_t<expiry_t>{expiry_t{1900_y / 1 / 1}});
    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "23 December 2020"sv) == res_t<expiry_t>{expiry_t{2020_y / 12 / 23}});

    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "23 December 20200"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "23 December 19"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "23 Octiber 19"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "32 October 19"sv) == parse_failure<expiry_t>);
    REQUIRE(test_parse<expiry_t>(p["NamedDate"], "0 October 19"sv) == parse_failure<expiry_t>);
}

TEST_CASE("TermLength")
{
    using namespace date;
    auto p = *prepare_parser();
    p["TermLength"].whitespaceOpe = p["License"].whitespaceOpe;
    REQUIRE(test_parse<expiry_t>(p["TermLength"], "01 day"sv) == res_t<expiry_t>{expiry_t{term_length_t{1,term_length_t::day}}});
    REQUIRE(test_parse<expiry_t>(p["TermLength"], "13 weeks"sv) == res_t<expiry_t>{expiry_t{term_length_t{13,term_length_t::week}}});
    REQUIRE(test_parse<expiry_t>(p["TermLength"], "23 months"sv) == res_t<expiry_t>{expiry_t{term_length_t{23,term_length_t::month}}});
    REQUIRE(test_parse<expiry_t>(p["TermLength"], "34year"sv) == res_t<expiry_t>{expiry_t{term_length_t{34,term_length_t::year}}});
    REQUIRE(test_parse<expiry_t>(p["TermLength"], "34 yar"sv) == parse_failure<expiry_t>);
}
#endif // !defined(DOCTEST_CONFIG_DISABLE)
