#ifndef __GST_DEEPDETECT_H__
#define __GST_DEEPDETECT_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <gst/cuda/gstcudamemory.h>
#include <memory>

G_BEGIN_DECLS

#define GST_TYPE_DEEPDETECT (gst_deepdetect_get_type())
G_DECLARE_FINAL_TYPE(GstDeepDetect, gst_deepdetect, GST, DEEPDETECT, GstBaseTransform)

/**
 * GstDeepDetect:
 * 
 * GPU-accelerated YOLOv8 object detection element using TensorRT
 */
struct _GstDeepDetect {
    GstBaseTransform parent;
    
    /* Properties */
    gchar *engine_path;
    gdouble score_threshold;
    gchar *int8_calib_cache;
    guint32 overlay_color;
    gboolean profile;
    
    /* Private data */
    gpointer priv;
    
    /* Metadata source pad */
    GstPad *src_meta;
};

/* Detection result structure */
typedef struct {
    gfloat x, y, w, h;  // Bounding box coordinates
    gint class_id;      // Class ID
    gfloat confidence;  // Detection confidence
} DetectionResult;

/* Helper function declarations */
void send_detection_metadata(GstDeepDetect *self, 
    const std::vector<DetectionResult> &detections, GstClockTime pts);

void add_overlay_composition(GstBuffer *buffer, 
    const std::vector<DetectionResult> &detections,
    guint32 color);

G_END_DECLS

#endif /* __GST_DEEPDETECT_H__ */