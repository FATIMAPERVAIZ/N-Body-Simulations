// Stage 6: compare two final-state CSVs (sequential vs MPI).
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Row {
    int id = -1;
    double x = 0, y = 0, vx = 0, vy = 0, m = 0;
};

static bool load_csv(const std::string& path, std::vector<Row>& rows, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot open " + path;
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {          // skip header
        error = "empty file: " + path;
        return false;
    }

    rows.clear();
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cell;
        Row r;

        if (!std::getline(ss, cell, ',')) continue;
        r.id = std::stoi(cell);

        if (!std::getline(ss, cell, ',')) continue;
        r.x = std::stod(cell);

        if (!std::getline(ss, cell, ',')) continue;
        r.y = std::stod(cell);

        if (!std::getline(ss, cell, ',')) continue;
        r.vx = std::stod(cell);

        if (!std::getline(ss, cell, ',')) continue;
        r.vy = std::stod(cell);

        if (!std::getline(ss, cell, ',')) continue;
        r.m = std::stod(cell);

        rows.push_back(r);
    }

    if (rows.empty()) {
        error = "no data rows in " + path;
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " sequential.csv mpi.csv [--tol 1e-6]\n";
        return 1;
    }

    const std::string path_a = argv[1];
    const std::string path_b = argv[2];
    double tol = 1e-6;

    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--tol" && i + 1 < argc) {
            tol = std::stod(argv[++i]);
        } else {
            std::cerr << "Unknown option\n";
            return 1;
        }
    }

    std::vector<Row> a, b;
    std::string error;
    if (!load_csv(path_a, a, error) || !load_csv(path_b, b, error)) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }
    if (a.size() != b.size()) {
        std::cerr << "Error: different body counts\n";
        return 2;
    }

    double max_dx = 0, max_dy = 0, max_dvx = 0, max_dvy = 0, max_dm = 0;
    int worst_id = -1;
    double worst_pos = 0;

    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id) {
            std::cerr << "id mismatch\n";
            return 2;
        }

        const double dx  = std::fabs(a[i].x  - b[i].x);
        const double dy  = std::fabs(a[i].y  - b[i].y);
        const double dvx = std::fabs(a[i].vx - b[i].vx);
        const double dvy = std::fabs(a[i].vy - b[i].vy);
        const double dm  = std::fabs(a[i].m  - b[i].m);

        max_dx  = std::max(max_dx,  dx);
        max_dy  = std::max(max_dy,  dy);
        max_dvx = std::max(max_dvx, dvx);
        max_dvy = std::max(max_dvy, dvy);
        max_dm  = std::max(max_dm,  dm);

        const double pos = std::max(dx, dy);
        if (pos > worst_pos) {
            worst_pos = pos;
            worst_id  = a[i].id;
        }
    }

    const bool ok = (std::max(max_dx, max_dy) <= tol) &&
                    (std::max(max_dvx, max_dvy) <= tol) &&
                    (max_dm <= tol);

    std::cout << std::setprecision(12)
              << "Compared " << a.size() << " bodies\n"
              << "  max |dx|  = " << max_dx << "\n"
              << "  max |dy|  = " << max_dy << "\n"
              << "  max |dvx| = " << max_dvx << "\n"
              << "  max |dvy| = " << max_dvy << "\n"
              << "  max |dm|  = " << max_dm << "\n"
              << "  worst body id = " << worst_id << "\n"
              << "  tolerance = " << tol << "\n"
              << (ok ? "PASS: states agree within tolerance\n"
                     : "FAIL: differences exceed tolerance\n");

    return ok ? 0 : 2;
}
