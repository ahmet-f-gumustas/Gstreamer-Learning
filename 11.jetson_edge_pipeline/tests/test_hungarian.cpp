#include "tracking/hungarian.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace edge;

static void test_square_optimal() {
    // 3x3 cost matrix; bilinen optimal = (0,1) + (1,0) + (2,2) = 1+1+1 = 3.
    std::vector<float> cost = {
        4, 1, 3,
        1, 4, 2,
        3, 2, 1
    };
    auto a = Hungarian::solve(cost, 3, 3);
    assert(a.size() == 3);
    float total = cost[0*3 + a[0]] + cost[1*3 + a[1]] + cost[2*3 + a[2]];
    assert(total <= 4.0f);   // optimal ≤ 4 (greedy 1+1+1=3 olmalı)
}

static void test_rectangular_more_cols() {
    // 2 rows, 3 cols → 2 atama yapılır, 1 sütun boş kalır.
    std::vector<float> cost = {
        10, 1, 5,
        2,  9, 8
    };
    auto a = Hungarian::solve(cost, 2, 3);
    assert(a.size() == 2);
    assert(a[0] == 1);   // row 0 → col 1 (cost 1)
    assert(a[1] == 0);   // row 1 → col 0 (cost 2)
}

static void test_infinite_cost_unassigned() {
    std::vector<float> cost = {
        Hungarian::INF_COST, Hungarian::INF_COST,
        1.f,                 Hungarian::INF_COST
    };
    auto a = Hungarian::solve(cost, 2, 2);
    assert(a[0] == -1);   // row 0 forbidden to all
    assert(a[1] == 0);
}

int main() {
    test_square_optimal();
    test_rectangular_more_cols();
    test_infinite_cost_unassigned();
    std::cout << "test_hungarian: OK\n";
    return 0;
}
