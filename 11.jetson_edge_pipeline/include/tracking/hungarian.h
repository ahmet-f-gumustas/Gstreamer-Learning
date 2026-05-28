#ifndef JETSON_EDGE_HUNGARIAN_H
#define JETSON_EDGE_HUNGARIAN_H

#include <vector>

namespace edge {

// Solves a rectangular linear assignment problem on a cost matrix
// of shape (rows x cols).  Returns the assignment for every row:
// out[i] = j  means row i is assigned to column j (-1 if unassigned).
//
// Cost values >= INF_COST are treated as forbidden (never assigned).
// Used by the ByteTrack association step over IoU costs.
class Hungarian {
public:
    static constexpr float INF_COST = 1e9f;

    // costs is row-major: costs[i * cols + j]
    static std::vector<int> solve(const std::vector<float>& costs,
                                  int rows, int cols);
};

}  // namespace edge

#endif
