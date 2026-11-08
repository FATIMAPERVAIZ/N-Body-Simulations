#include "barnes_hut.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace {

struct Node {
    double xmin, xmax, ymin, ymax;
    double mass = 0.0;
    double cx = 0.0, cy = 0.0;
    int body_idx = -1;          // >=0 only for leaf with one body
    bool leaf = true;
    std::unique_ptr<Node> child[4];

    Node(double x0, double x1, double y0, double y1)
        : xmin(x0), xmax(x1), ymin(y0), ymax(y1) {}

    double side() const { return xmax - xmin; }

    int quad(double x, double y) const {
        double mx = 0.5 * (xmin + xmax);
        double my = 0.5 * (ymin + ymax);
        int q = 0;
        if (x >= mx) q |= 1;
        if (y >= my) q |= 2;
        return q;
    }

    void subdivide() {
        double mx = 0.5 * (xmin + xmax);
        double my = 0.5 * (ymin + ymax);
        child[0] = std::make_unique<Node>(xmin, mx, ymin, my);
        child[1] = std::make_unique<Node>(mx, xmax, ymin, my);
        child[2] = std::make_unique<Node>(xmin, mx, my, ymax);
        child[3] = std::make_unique<Node>(mx, xmax, my, ymax);
        leaf = false;
    }
};

void insert(Node* node, int idx, const std::vector<Body>& b) {
    const Body& body = b[idx];
    double mnew = node->mass + body.m;
    if (mnew > 0.0) {
        node->cx = (node->cx * node->mass + body.x * body.m) / mnew;
        node->cy = (node->cy * node->mass + body.y * body.m) / mnew;
    }
    node->mass = mnew;

    if (node->leaf) {
        if (node->body_idx == -1) {
            node->body_idx = idx;
            return;
        }
        // split
        int old = node->body_idx;
        node->body_idx = -1;
        node->subdivide();
        insert(node, old, b);
        insert(node, idx, b);
    } else {
        insert(node->child[node->quad(body.x, body.y)].get(), idx, b);
    }
}

void force_from_node(const Node* node, int i, const std::vector<Body>& b,
                     double G, double theta, double eps2,
                     double& ax, double& ay) {
    if (!node || node->mass == 0.0) return;

    const Body& bi = b[i];
    double dx = node->cx - bi.x;
    double dy = node->cy - bi.y;
    double d2 = dx*dx + dy*dy + eps2;
    double d  = std::sqrt(d2);

    if (node->leaf) {
        if (node->body_idx == i) return;
        double inv = G * node->mass / (d2 * d);
        ax += inv * dx;
        ay += inv * dy;
        return;
    }

    // Barnes-Hut opening criterion
    if (node->side() / d < theta) {
        double inv = G * node->mass / (d2 * d);
        ax += inv * dx;
        ay += inv * dy;
    } else {
        for (int k = 0; k < 4; ++k)
            force_from_node(node->child[k].get(), i, b, G, theta, eps2, ax, ay);
    }
}

} // namespace

void barnes_hut_accelerations(
    const std::vector<Body>& bodies,
    std::vector<double>& ax,
    std::vector<double>& ay,
    const SimConfig& cfg,
    double theta)
{
    const int n = static_cast<int>(bodies.size());
    ax.assign(n, 0.0);
    ay.assign(n, 0.0);
    if (n == 0) return;

    // bounding square
    double xmin = bodies[0].x, xmax = bodies[0].x;
    double ymin = bodies[0].y, ymax = bodies[0].y;
    for (const auto& b : bodies) {
        xmin = std::min(xmin, b.x); xmax = std::max(xmax, b.x);
        ymin = std::min(ymin, b.y); ymax = std::max(ymax, b.y);
    }
    double size = std::max(xmax - xmin, ymax - ymin) * 1.02 + 1e-5;
    double cx = 0.5 * (xmin + xmax);
    double cy = 0.5 * (ymin + ymax);

    Node root(cx - 0.5*size, cx + 0.5*size, cy - 0.5*size, cy + 0.5*size);
    for (int i = 0; i < n; ++i) insert(&root, i, bodies);

    const double eps2 = (2.0 * cfg.radius) * (2.0 * cfg.radius);

    for (int i = 0; i < n; ++i) {
        double axi = 0.0, ayi = 0.0;
        force_from_node(&root, i, bodies, cfg.gravity, theta, eps2, axi, ayi);
        ax[i] = axi;
        ay[i] = ayi;
    }
}
