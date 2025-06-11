#include "nvoverlay.hh"
#include "gstdeepdetect.h"
#include <gst/video/video.h>

void add_overlay_composition(GstBuffer *buffer, 
                           const std::vector<DetectionResult> &detections,
                           guint32 color) {
    
    if (detections.empty()) return;
    
    GstVideoMeta *video_meta = gst_buffer_get_video_meta(buffer);
    if (!video_meta) return;
    
    gint width = video_meta->width;
    gint height = video_meta->height;
    
    std::vector<GstVideoOverlayRectangle*> rectangles;
    
    for (const auto &detection : detections) {
        GstVideoOverlayRectangle *rect = create_overlay_rectangle(
            detection, color, width, height);
        if (rect) {
            rectangles.push_back(rect);
        }
    }
    
    if (!rectangles.empty()) {
        GstVideoOverlayComposition *comp = 
            gst_video_overlay_composition_new(rectangles[0]);
        
        for (size_t i = 1; i < rectangles.size(); i++) {
            gst_video_overlay_composition_add_rectangle(comp, rectangles[i]);
        }
        
        gst_buffer_add_video_overlay_composition_meta(buffer, comp);
        gst_video_overlay_composition_unref(comp);
        
        // Unref rectangles
        for (auto rect : rectangles) {
            gst_video_overlay_rectangle_unref(rect);
        }
    }
}

GstVideoOverlayRectangle* create_overlay_rectangle(
    const DetectionResult &detection,
    guint32 color,
    gint video_width,
    gint video_height) {
    
    // Convert normalized coordinates to pixel coordinates
    gint x = (gint)(detection.x * video_width);
    gint y = (gint)(detection.y * video_height);
    gint w = (gint)(detection.w * video_width);
    gint h = (gint)(detection.h * video_height);
    
    // Clamp to video boundaries
    x = CLAMP(x, 0, video_width - 1);
    y = CLAMP(y, 0, video_height - 1);
    w = CLAMP(w, 1, video_width - x);
    h = CLAMP(h, 1, video_height - y);
    
    // Create a simple colored rectangle buffer
    GstBuffer *rect_buffer = gst_buffer_new_allocate(NULL, w * h * 4, NULL);
    
    GstMapInfo map;
    gst_buffer_map(rect_buffer, &map, GST_MAP_WRITE);
    
    // Fill with border color (hollow rectangle)
    guint32 *pixels = (guint32*)map.data;
    for (gint row = 0; row < h; row++) {
        for (gint col = 0; col < w; col++) {
            gboolean is_border = (row < 2 || row >= h - 2 || 
                                 col < 2 || col >= w - 2);
            pixels[row * w + col] = is_border ? color : 0x00000000; // Transparent inside
        }
    }
    
    gst_buffer_unmap(rect_buffer, &map);
    
    return gst_video_overlay_rectangle_new_raw(rect_buffer, x, y, w, h,
        GST_VIDEO_OVERLAY_FORMAT_FLAG_NONE);
}