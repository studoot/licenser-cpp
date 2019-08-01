#ifndef LICENSE_FORMATTERS_HPP
#define LICENSE_FORMATTERS_HPP

#include "license.hpp"

#include <ostream>

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace std
{
template <class T>
std::ostream &operator<<(std::ostream &os, const vector<T> &value)
{
    os << "[ ";
    for (const auto &i : value)
        os << i << (&i == &value.back() ? "" : ", ");
    return os << " ]";
}
std::ostream &operator<<(std::ostream &os, const expiry_t &value)
{
    return std::visit(overloaded{[&](const perpetual_t &) -> std::ostream & { return os << "Expiry{Perpetual}"; },
                                 [&](const term_length_t &t) -> std::ostream & {
                                     using namespace std::literals;
                                     static const auto unit_names =
                                         std::unordered_map<term_length_t::units_t,
                                                            std::string>{{term_length_t::day, "day"s},
                                                                         {term_length_t::week, "week"s},
                                                                         {term_length_t::month, "month"s},
                                                                         {term_length_t::year, "year"s}};
                                     return os << fmt::format("Expiry{{{} {}{}}}", t.count, unit_names.at(t.units),
                                                              t.count != 1 ? "s" : "");
                                 },
                                 [&](const date::year_month_day &ymd) -> std::ostream & {
                                     return os << fmt::format("Expiry{{{}}}", ymd);
                                 }},
                      value);
}
std::ostream &operator<<(std::ostream &os, const location_t &value)
{
    return std::visit(overloaded{[&](const anywhere_t &) -> std::ostream & {
                                     return os << fmt::format("Location{{Anywhere}}");
                                 },
                                 [&](const node_t &t) -> std::ostream & {
                                     return os << fmt::format("Location{{Node {}}}", t.get());
                                 }},
                      value);
}
std::ostream &operator<<(std::ostream &os, const identity_t &value)
{
    return std::visit(overloaded{[&](const anyone_t &) -> std::ostream & {
                                     return os << fmt::format("Identity{{Anyone}}");
                                 },
                                 [&](const user_t &t) -> std::ostream & {
                                     return os << fmt::format("Identity{{User {}}}", t.get());
                                 },
                                 [&](const domain_t &t) -> std::ostream & {
                                     return os << fmt::format("Identity{{Domain {}}}", t.get());
                                 }},
                      value);
}
} // namespace std

#endif /* LICENSE_FORMATTERS_HPP */
