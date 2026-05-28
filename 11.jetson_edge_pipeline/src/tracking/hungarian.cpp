#include "tracking/hungarian.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace edge {

// Jonker-Volgenant variant of the rectangular Hungarian algorithm.
// Reference: Crouse 2016 (DOI 10.1109/TAES.2016.140952).
// O((n+m)^3) but very fast for the small (<200x200) cost matrices we see.
std::vector<int> Hungarian::solve(const std::vector<float>& cost,
                                  int rows, int cols) {
    if (rows == 0 || cols == 0) return std::vector<int>(rows, -1);

    int n = std::max(rows, cols);
    const float INF = std::numeric_limits<float>::infinity();

    // Pad to square with INF
    std::vector<std::vector<float>> a(n, std::vector<float>(n, INF));
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            a[i][j] = cost[i * cols + j];

    std::vector<float> u(n + 1, 0), v(n + 1, 0);
    std::vector<int>   p(n + 1, 0), way(n + 1, 0);

    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::vector<float> minv(n + 1, INF);
        std::vector<bool>  used(n + 1, false);

        do {
            used[j0] = true;
            int i0 = p[j0];
            float delta = INF;
            int j1 = -1;
            for (int j = 1; j <= n; ++j) if (!used[j]) {
                float cur = a[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }
            for (int j = 0; j <= n; ++j) {
                if (used[j])      { u[p[j]] += delta; v[j] -= delta; }
                else              minv[j]  -= delta;
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }

    std::vector<int> out(rows, -1);
    for (int j = 1; j <= n; ++j) {
        int i = p[j] - 1;
        int jj = j - 1;
        if (i < rows && jj < cols && a[i][jj] < INF_COST)
            out[i] = jj;
    }
    return out;
}

}  // namespace edge
