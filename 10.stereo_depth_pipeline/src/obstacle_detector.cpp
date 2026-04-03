#include "obstacle_detector.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

// ─────────────────────────────────────────────────────────────────────────────
ObstacleDetector::ObstacleDetector(float dangerM, float cautionM,
                                   int gridRows, int gridCols)
    : dangerM_(dangerM), cautionM_(cautionM),
      gridRows_(gridRows), gridCols_(gridCols)
{}

// ─── Classification ──────────────────────────────────────────────────────────
DangerLevel ObstacleDetector::classify(float dist) const
{
    if (dist <= 0.f)       return DangerLevel::SAFE;   // invalid pixel
    if (dist <= dangerM_)  return DangerLevel::DANGER;
    if (dist <= cautionM_) return DangerLevel::CAUTION;
    return DangerLevel::SAFE;
}

cv::Scalar ObstacleDetector::levelColor(DangerLevel l) const
{
    switch (l) {
        case DangerLevel::DANGER:  return cv::Scalar(0,   0, 220);   // Red
        case DangerLevel::CAUTION: return cv::Scalar(0, 165, 255);   // Orange
        default:                   return cv::Scalar(0, 180,   0);   // Green
    }
}

std::string ObstacleDetector::levelLabel(DangerLevel l) const
{
    switch (l) {
        case DangerLevel::DANGER:  return "DANGER";
        case DangerLevel::CAUTION: return "CAUTION";
        default:                   return "SAFE";
    }
}

// ─── Grid analysis ───────────────────────────────────────────────────────────
std::vector<GridCell> ObstacleDetector::buildGrid(const cv::Mat& depthMap) const
{
    std::vector<GridCell> cells;
    int cellH = depthMap.rows / gridRows_;
    int cellW = depthMap.cols / gridCols_;

    for (int r = 0; r < gridRows_; ++r) {
        for (int c = 0; c < gridCols_; ++c) {
            cv::Rect roi(c * cellW, r * cellH, cellW, cellH);
            cv::Mat  patch = depthMap(roi);

            // Get the average of valid (> 0) pixels
            std::vector<float> valid;
            valid.reserve(static_cast<size_t>(patch.total()));
            for (int y = 0; y < patch.rows; ++y) {
                const float* row = patch.ptr<float>(y);
                for (int x = 0; x < patch.cols; ++x)
                    if (row[x] > 0.01f) valid.push_back(row[x]);
            }

            float avg = 0.f;
            if (!valid.empty()) {
                // Use the lowest 10th percentile: prioritize closest objects
                std::sort(valid.begin(), valid.end());
                size_t n = std::max<size_t>(1, valid.size() / 10);
                avg = std::accumulate(valid.begin(), valid.begin() + static_cast<long>(n), 0.f) / n;
            }

            GridCell cell;
            cell.row      = r;
            cell.col      = c;
            cell.avgDepthM = avg;
            cell.level    = classify(avg);
            cell.rect     = roi;
            cells.push_back(cell);
        }
    }
    return cells;
}

// ─── Blob-based obstacle detection ───────────────────────────────────────────
std::vector<Obstacle> ObstacleDetector::blobObstacles(const cv::Mat& depthMap) const
{
    // Mask pixels in the DANGER region
    cv::Mat dangerMask;
    cv::inRange(depthMap, 0.01f, dangerM_, dangerMask);

    // Morphological cleanup
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::morphologyEx(dangerMask, dangerMask, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(dangerMask, dangerMask, cv::MORPH_OPEN,  kernel);

    // Connected components
    cv::Mat labels, stats, centroids;
    int n = cv::connectedComponentsWithStats(dangerMask, labels, stats, centroids);

    std::vector<Obstacle> obstacles;
    for (int i = 1; i < n; ++i) {   // i=0 is background
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area < 500) continue;   // skip very small blobs

        cv::Rect bbox(
            stats.at<int>(i, cv::CC_STAT_LEFT),
            stats.at<int>(i, cv::CC_STAT_TOP),
            stats.at<int>(i, cv::CC_STAT_WIDTH),
            stats.at<int>(i, cv::CC_STAT_HEIGHT)
        );

        // Average depth within the blob
        cv::Mat blobMask = (labels == i);
        std::vector<float> vals;
        for (int y = bbox.y; y < bbox.y + bbox.height; ++y) {
            for (int x = bbox.x; x < bbox.x + bbox.width; ++x) {
                if (blobMask.at<uchar>(y, x) && depthMap.at<float>(y, x) > 0.f)
                    vals.push_back(depthMap.at<float>(y, x));
            }
        }

        float dist = vals.empty() ? 0.f :
            *std::min_element(vals.begin(), vals.end());

        std::ostringstream label;
        label << std::fixed << std::setprecision(2) << dist << "m";

        Obstacle obs;
        obs.bbox      = bbox;
        obs.distanceM = dist;
        obs.level     = DangerLevel::DANGER;
        obs.label     = label.str();
        obstacles.push_back(obs);
    }
    return obstacles;
}

// ─── Overlay drawing ─────────────────────────────────────────────────────────
cv::Mat ObstacleDetector::drawOverlay(const cv::Mat& colorFrame,
                                      const std::vector<GridCell>& grid,
                                      const std::vector<Obstacle>& obs,
                                      float closestM,
                                      DangerLevel overall) const
{
    cv::Mat out = colorFrame.clone();

    // ── Grid cells ────────────────────────────────────────────────────────────
    for (const auto& cell : grid) {
        cv::Scalar color = levelColor(cell.level);
        // Semi-transparent rectangle
        cv::Mat roi = out(cell.rect);
        cv::Mat fillColor(roi.size(), roi.type(), color * 0.3);
        cv::addWeighted(roi, 0.7, fillColor, 0.3, 0, roi);
        cv::rectangle(out, cell.rect, color, 2);

        // Average depth label
        if (cell.avgDepthM > 0.f) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << cell.avgDepthM << "m";
            int tx = cell.rect.x + 4;
            int ty = cell.rect.y + cell.rect.height - 6;
            cv::putText(out, ss.str(), cv::Point(tx, ty),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,255,255), 2);
            cv::putText(out, ss.str(), cv::Point(tx, ty),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
    }

    // ── Blob obstacles ────────────────────────────────────────────────────────
    for (const auto& o : obs) {
        cv::Scalar color = levelColor(o.level);
        cv::rectangle(out, o.bbox, color, 3);
        cv::putText(out, "OBSTACLE " + o.label,
                    cv::Point(o.bbox.x, o.bbox.y - 6),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
    }

    // ── Top info bar ──────────────────────────────────────────────────────────
    cv::rectangle(out, cv::Rect(0, 0, out.cols, 36),
                  cv::Scalar(20, 20, 20), cv::FILLED);

    // Overall status
    std::string status = levelLabel(overall);
    cv::putText(out, "STATUS: " + status, cv::Point(8, 24),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, levelColor(overall), 2);

    // Closest obstacle
    if (closestM < 900.f) {
        std::ostringstream ss;
        ss << "Closest: " << std::fixed << std::setprecision(2) << closestM << " m";
        cv::putText(out, ss.str(), cv::Point(out.cols / 2 - 90, 24),
                    cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(200, 200, 200), 2);
    }

    // Threshold info
    std::ostringstream threshStr;
    threshStr << "Thresh: DANGER<" << dangerM_ << "m  CAUTION<" << cautionM_ << "m";
    cv::putText(out, threshStr.str(), cv::Point(out.cols - 360, 24),
                cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(160, 160, 160), 1);

    return out;
}

// ─── Main analysis function ──────────────────────────────────────────────────
ObstacleResult ObstacleDetector::analyze(const cv::Mat& depthMap,
                                         const cv::Mat& colorFrame)
{
    ObstacleResult result;
    if (depthMap.empty() || colorFrame.empty()) return result;

    // Grid
    result.grid = buildGrid(depthMap);

    // Closest cell
    float closest = 999.f;
    for (const auto& cell : result.grid) {
        if (cell.avgDepthM > 0.f && cell.avgDepthM < closest)
            closest = cell.avgDepthM;
    }
    result.closestM = closest;

    // Overall danger level (worst cell)
    result.overallDanger = DangerLevel::SAFE;
    for (const auto& cell : result.grid) {
        if (cell.level == DangerLevel::DANGER) {
            result.overallDanger = DangerLevel::DANGER;
            break;
        }
        if (cell.level == DangerLevel::CAUTION)
            result.overallDanger = DangerLevel::CAUTION;
    }

    // Blob obstacles
    result.obstacles = blobObstacles(depthMap);

    // Visual
    result.overlay = drawOverlay(colorFrame, result.grid,
                                  result.obstacles, closest, result.overallDanger);
    return result;
}
