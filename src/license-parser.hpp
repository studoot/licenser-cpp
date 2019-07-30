#ifndef LICENSE_PARSER_HPP
#define LICENSE_PARSER_HPP

#include "license.hpp"

#include <optional>
#include <string>
#include <string_view>

std::optional<license_t> parse_license(const date::year_month_day &eval_date, std::string_view text, std::optional<std::string> const &from_file);

#endif /* LICENSE_PARSER_HPP */
