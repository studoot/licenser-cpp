#include "license.hpp"
#include "license-parser.hpp"
#include "license-formatters.hpp"

#include <fstream>
#include <sstream>

#include <fmt/core.h>
#include <fmt/ostream.h>

std::string read_file(std::string const &filename)
{
    if (auto in = std::ifstream(filename, std::ios::in | std::ios::binary))
    {
        std::ostringstream contents;
        contents << in.rdbuf();
        return (contents.str());
    }
    return std::string{};
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        const auto file = read_file(argv[1]);
        const auto today = date::year_month_day{
            date::floor<date::days>(std::chrono::system_clock::now())};
        if (const auto license = parse_license(today, file, argv[1]))
        {
            fmt::print("License secret = {}, expiry = {}, locn = {}, id = {}\n",
                       license->secret, license->expiry, license->allowed_places,
                       license->allowed_users);
        }
    }
}
