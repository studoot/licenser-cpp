#include "license-parser.hpp"

#include "license.peg.h"
#include "overloaded.hpp"

#include <peglib.h>
using namespace peg;

uint16_t to_natural(const SemanticValues &sv) { return std::stoul(sv.str()); }

term_length_t::units_t to_term_unit(const SemanticValues &sv)
{
    return static_cast<term_length_t::units_t>(sv.choice());
}

uint16_t from_month_name(const SemanticValues &sv)
{
    return static_cast<uint16_t>(sv.choice() + 1);
}

std::optional<license_t> parse_license(const date::year_month_day &eval_date, std::string_view text, std::optional<std::string> const &from_file)
{
    parser p;

    if (!p.load_grammar(reinterpret_cast<const char *>(license_peg)))
        return std::nullopt;

    p["NATURAL"] = to_natural;

    p["License"] = [&](const SemanticValues &sv) {
        license_t license;
        license.terms = sv.transform<license_term_t>();
        for (const auto &term : license.terms)
        {
            license.process_term(eval_date, term);
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
    license_t license;
    p.parse_n(text.data(), text.size(), license, from_file.value_or("").c_str());
    return license;
}
