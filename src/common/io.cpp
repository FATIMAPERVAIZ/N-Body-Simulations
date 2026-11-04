#include "io.hpp"

#include <filesystem>
#include <iomanip>
#include <limits>

namespace {

bool make_parent_folder(const std::string& path, std::string& error) {
    const std::filesystem::path p(path);
    if (!p.has_parent_path()) return true;
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec) {
        error = "cannot create folder " + p.parent_path().string() + ": " + ec.message();
        return false;
    }
    return true;
}

}  // namespace

bool save_state_csv(const std::string& path, const std::vector<Body>& bodies, std::string& error) {
    if (!make_parent_folder(path, error)) return false;
    std::ofstream out(path);
    if (!out) {
        error = "cannot open " + path + " for writing";
        return false;
    }
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "id,x,y,vx,vy,m\n";
    for (size_t i = 0; i < bodies.size(); ++i) {
        const Body& b = bodies[i];
        out << i << ',' << b.x << ',' << b.y << ',' << b.vx << ',' << b.vy << ',' << b.m << '\n';
    }
    if (!out) {
        error = "write failed for " + path;
        return false;
    }
    return true;
}

bool open_snapshot_file(const std::string& path, std::ofstream& out, std::string& error) {
    if (!make_parent_folder(path, error)) return false;
    out.open(path);
    if (!out) {
        error = "cannot open " + path + " for writing";
        return false;
    }
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "step,id,x,y,vx,vy,m\n";
    return true;
}

void append_snapshot(std::ofstream& out, int step, const std::vector<Body>& bodies) {
    for (size_t i = 0; i < bodies.size(); ++i) {
        const Body& b = bodies[i];
        out << step << ',' << i << ',' << b.x << ',' << b.y << ',' << b.vx << ',' << b.vy
            << ',' << b.m << '\n';
    }
}
