#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// ─── Danger levels ───────────────────────────────────────────────────────────
enum class DangerLevel {
    SAFE,       // > cautionDist
    CAUTION,    // dangerDist < distance <= cautionDist
    DANGER      // <= dangerDist
};

// ─── Single obstacle ─────────────────────────────────────────────────────────
struct Obstacle {
    cv::Rect    bbox;
    float       distanceM;
    DangerLevel level;
    std::string label;
};

// ─── Grid cell ───────────────────────────────────────────────────────────────
struct GridCell {
    int         row, col;
    float       avgDepthM;
    DangerLevel level;
    cv::Rect    rect;
};

// ─── Analysis result ─────────────────────────────────────────────────────────
struct ObstacleResult {
    std::vector<Obstacle>  obstacles;
    std::vector<GridCell>  grid;
    float                  closestM      = 999.f;
    DangerLevel            overallDanger = DangerLevel::SAFE;
    cv::Mat                overlay;    // color-layered image
};

// ─── ObstacleDetector ────────────────────────────────────────────────────────
//
// Divides the depth map into grid cells and classifies each cell as
// SAFE / CAUTION / DANGER based on the average depth within the cell.
//
// Additionally extracts bounding boxes from large obstacles via blob analysis.
//
class ObstacleDetector {
public:
    explicit ObstacleDetector(float dangerM  = 1.0f,
                              float cautionM = 3.0f,
                              int   gridRows = 3,
                              int   gridCols = 4);

    ObstacleResult analyze(const cv::Mat& depthMap,
                           const cv::Mat& colorFrame);

private:
    float dangerM_;
    float cautionM_;
    int   gridRows_;
    int   gridCols_;

    DangerLevel classify(float dist) const;
    cv::Scalar  levelColor(DangerLevel l) const;
    std::string levelLabel(DangerLevel l) const;

    std::vector<GridCell> buildGrid(const cv::Mat& depthMap) const;
    std::vector<Obstacle> blobObstacles(const cv::Mat& depthMap) const;
    cv::Mat               drawOverlay(const cv::Mat& colorFrame,
                                      const std::vector<GridCell>& grid,
                                      const std::vector<Obstacle>& obs,
                                      float closestM,
                                      DangerLevel overall) const;
};
