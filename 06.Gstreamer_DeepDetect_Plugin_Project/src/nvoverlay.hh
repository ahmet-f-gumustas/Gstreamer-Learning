#ifndef __NVOVERLAY_HH__
#define __NVOVERLAY_HH__

#include <gst/video/video.h>
#include <gst/video/gstvideooverlay.h>
#include <vector>

struct DetectionResult;

/**
 * @brief Add overlay composition with bounding boxes to GstBuffer
 * @param buffer Video buffer to add overlay to
 * @param detections Vector of detection results
 * @param color RGBA color for bounding boxes
 */
void add_overlay_composition(GstBuffer *buffer, 
                           const std::vector<DetectionResult> &detections,
                           guint32 color);

/**
 * @brief Create a single overlay rectangle for a detection
 * @param detection Detection result
 * @param color RGBA color
 * @param video_width Video frame width
 * @param video_height Video frame height
 * @return GstVideoOverlayRectangle
 */
GstVideoOverlayRectangle* create_overlay_rectangle(
    const DetectionResult &detection,
    guint32 color,
    gint video_width,
    gint video_height);

#endif /* __NVOVERLAY_HH__ */