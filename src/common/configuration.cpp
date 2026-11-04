#include "configuration.hpp"

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

bool parse_int(const char* text, int& out) {
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE) return false;
    if (value < INT_MIN || value > INT_MAX) return false;
    out = static_cast<int>(value);
    return true;
}

bool parse_u64(const char* text, std::uint64_t& out) {
    if (text[0] == '-' || text[0] == '\0') return false;
    char* end = nullptr;
    errno = 0;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE) return false;
    out = static_cast<std::uint64_t>(value);
    return true;
}

bool parse_double(const char* text, double& out) {
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE) return false;
    if (!std::isfinite(value)) return false;
    out = value;
    return true;
}

}  // namespace

bool parse_config(int argc, char** argv, SimConfig& cfg, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h") {
            cfg.show_help = true;
            return true;
        }
        if (i + 1 >= argc) {
            error = "missing value for " + key;
            return false;
        }
        const char* value = argv[++i];
        bool ok = true;

        if (key == "--bodies")              ok = parse_int(value, cfg.bodies);
        else if (key == "--steps")          ok = parse_int(value, cfg.steps);
        else if (key == "--seed")           ok = parse_u64(value, cfg.seed);
        else if (key == "--dt")             ok = parse_double(value, cfg.dt);
        else if (key == "--gravity")        ok = parse_double(value, cfg.gravity);
        else if (key == "--space")          ok = parse_double(value, cfg.space);
        else if (key == "--radius")         ok = parse_double(value, cfg.radius);
        else if (key == "--min_mass")       ok = parse_double(value, cfg.min_mass);
        else if (key == "--max_mass")       ok = parse_double(value, cfg.max_mass);
        else if (key == "--snapshot_every") ok = parse_int(value, cfg.snapshot_every);
        else if (key == "--output")         cfg.output_file = value;
        else if (key == "--snapshot_file")  cfg.snapshot_file = value;
        else {
            error = "unknown option " + key;
            return false;
        }
        if (!ok) {
            error = "invalid value '" + std::string(value) + "' for " + key;
            return false;
        }
    }
    return true;
}

bool validate_config(const SimConfig& cfg, std::string& error) {
    if (cfg.bodies < 1 || cfg.bodies > 1000000) { error = "bodies must be between 1 and 1000000"; return false; }
    if (cfg.steps < 0)                          { error = "steps must be 0 or more"; return false; }
    if (!(cfg.dt > 0.0))                        { error = "dt must be greater than 0"; return false; }
    if (cfg.gravity < 0.0)                      { error = "gravity must be 0 or more"; return false; }
    if (!(cfg.radius > 0.0))                    { error = "radius must be greater than 0"; return false; }
    if (!(cfg.space > 2.0 * cfg.radius))        { error = "space must be larger than 2 * radius"; return false; }
    if (!(cfg.min_mass > 0.0))                  { error = "min_mass must be greater than 0"; return false; }
    if (cfg.max_mass < cfg.min_mass)            { error = "max_mass must be at least min_mass"; return false; }
    if (cfg.snapshot_every < 0)                 { error = "snapshot_every must be 0 or more"; return false; }
    return true;
}

void print_usage(const char* program) {
    const SimConfig d;
    std::cout << "Usage: " << program << " [options]\n"
              << "  --bodies N          number of bodies         (default " << d.bodies << ")\n"
              << "  --steps N           number of timesteps      (default " << d.steps << ")\n"
              << "  --seed N            random seed              (default " << d.seed << ")\n"
              << "  --dt X              timestep length          (default " << d.dt << ")\n"
              << "  --gravity X         gravitational constant   (default " << d.gravity << ")\n"
              << "  --space X           world size               (default " << d.space << ")\n"
              << "  --radius X          body radius              (default " << d.radius << ")\n"
              << "  --min_mass X        smallest mass            (default " << d.min_mass << ")\n"
              << "  --max_mass X        largest mass             (default " << d.max_mass << ")\n"
              << "  --snapshot_every N  save a snapshot every N steps, 0 = off (default 0)\n"
              << "  --output FILE       final state CSV\n"
              << "  --snapshot_file FILE snapshot CSV\n"
              << "  --help              show this text\n";
}
