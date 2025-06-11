/**
 * @file gstdeepdetect.cc
 * @brief GPU-zero-copy TensorRT YOLOv8 GStreamer Transform implementation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstdeepdetect.h"
#include "yolo_engine.hh"
#include "nvoverlay.hh"

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <gst/video/gstvideooverlay.h>
#include <gst/cuda/gstcudamemory.h>
#include <nlohmann/json.hpp>
#include <nvtx3/nvToolsExt.h>

GST_DEBUG_CATEGORY_STATIC(gst_deepdetect_debug);
#define GST_CAT_DEFAULT gst_deepdetect_debug

/* Properties */
enum {
    PROP_0,
    PROP_ENGINE_PATH,
    PROP_SCORE_THRESHOLD,
    PROP_INT8_CALIB_CACHE,
    PROP_OVERLAY_COLOR,
    PROP_PROFILE
};

/* Pad templates */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "video/x-raw(memory:NVMM), "
        "format=(string){ NV12, RGBA }, "
        "width=[1,8192], height=[1,8192], "
        "framerate=[1/1,240/1]"
    )
);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "video/x-raw(memory:NVMM), "
        "format=(string){ NV12, RGBA }, "
        "width=[1,8192], height=[1,8192], "
        "framerate=[1/1,240/1]"
    )
);

static GstStaticPadTemplate src_meta_template = GST_STATIC_PAD_TEMPLATE(
    "src_meta",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/json")
);

/* Private structure */
struct DeepDetectPrivate {
    std::unique_ptr<YoloEngine> engine;
    GstCudaContext *cuda_context;
    cudaStream_t stream;
    GstVideoInfo video_info;
    gboolean initialized;
};

#define GST_DEEPDETECT_GET_PRIVATE(obj) \
    ((DeepDetectPrivate*)((obj)->priv))

/* Function prototypes */
static void gst_deepdetect_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec);
static void gst_deepdetect_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);
static void gst_deepdetect_finalize(GObject *object);

static gboolean gst_deepdetect_start(GstBaseTransform *trans);
static gboolean gst_deepdetect_stop(GstBaseTransform *trans);
static gboolean gst_deepdetect_set_caps(GstBaseTransform *trans,
    GstCaps *incaps, GstCaps *outcaps);
static GstFlowReturn gst_deepdetect_transform_ip(GstBaseTransform *trans,
    GstBuffer *buf);

G_DEFINE_TYPE_WITH_PRIVATE(GstDeepDetect, gst_deepdetect, GST_TYPE_BASE_TRANSFORM);

static void gst_deepdetect_class_init(GstDeepDetectClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass *transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    
    /* Set virtual functions */
    gobject_class->set_property = gst_deepdetect_set_property;
    gobject_class->get_property = gst_deepdetect_get_property;
    gobject_class->finalize = gst_deepdetect_finalize;
    
    transform_class->start = gst_deepdetect_start;
    transform_class->stop = gst_deepdetect_stop;
    transform_class->set_caps = gst_deepdetect_set_caps;
    transform_class->transform_ip = gst_deepdetect_transform_ip;
    
    /* Install properties */
    g_object_class_install_property(gobject_class, PROP_ENGINE_PATH,
        g_param_spec_string("engine-path", "Engine Path",
            "Path to TensorRT engine file", NULL,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_SCORE_THRESHOLD,
        g_param_spec_double("score-threshold", "Score Threshold",
            "Detection score threshold", 0.0, 1.0, 0.25,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_INT8_CALIB_CACHE,
        g_param_spec_string("int8-calib-cache", "INT8 Calibration Cache",
            "Path to INT8 calibration cache file", NULL,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_OVERLAY_COLOR,
        g_param_spec_uint("overlay-color", "Overlay Color",
            "RGBA color for bounding box overlay", 0, G_MAXUINT32, 0xFF0000FF,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_PROFILE,
        g_param_spec_boolean("profile", "Profile",
            "Enable NVTX profiling", FALSE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    /* Set element metadata */
    gst_element_class_set_static_metadata(element_class,
        "DeepDetect YOLOv8 Object Detection",
        "Filter/Effect/Video",
        "GPU-accelerated object detection using TensorRT YOLOv8",
        "Your Name <your.email@example.com>");
    
    /* Add pad templates */
    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);
    gst_element_class_add_static_pad_template(element_class, &src_meta_template);
}

static void gst_deepdetect_init(GstDeepDetect *self) {
    self->engine_path = NULL;
    self->score_threshold = 0.25;
    self->int8_calib_cache = NULL;
    self->overlay_color = 0xFF0000FF; // Red
    self->profile = FALSE;
    
    self->priv = g_new0(DeepDetectPrivate, 1);
    DeepDetectPrivate *priv = GST_DEEPDETECT_GET_PRIVATE(self);
    priv->initialized = FALSE;
    
    /* Create metadata source pad */
    self->src_meta = gst_pad_new_from_static_template(&src_meta_template, "src_meta");
    gst_element_add_pad(GST_ELEMENT(self), self->src_meta);
}

static void gst_deepdetect_finalize(GObject *object) {
    GstDeepDetect *self = GST_DEEPDETECT(object);
    
    g_free(self->engine_path);
    g_free(self->int8_calib_cache);
    g_free(self->priv);
    
    G_OBJECT_CLASS(gst_deepdetect_parent_class)->finalize(object);
}

static void gst_deepdetect_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec) {
    GstDeepDetect *self = GST_DEEPDETECT(object);
    
    switch (prop_id) {
        case PROP_ENGINE_PATH:
            g_free(self->engine_path);
            self->engine_path = g_value_dup_string(value);
            break;
        case PROP_SCORE_THRESHOLD:
            self->score_threshold = g_value_get_double(value);
            break;
        case PROP_INT8_CALIB_CACHE:
            g_free(self->int8_calib_cache);
            self->int8_calib_cache = g_value_dup_string(value);
            break;
        case PROP_OVERLAY_COLOR:
            self->overlay_color = g_value_get_uint(value);
            break;
        case PROP_PROFILE:
            self->profile = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_deepdetect_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec) {
    GstDeepDetect *self = GST_DEEPDETECT(object);
    
    switch (prop_id) {
        case PROP_ENGINE_PATH:
            g_value_set_string(value, self->engine_path);
            break;
        case PROP_SCORE_THRESHOLD:
            g_value_set_double(value, self->score_threshold);
            break;
        case PROP_INT8_CALIB_CACHE:
            g_value_set_string(value, self->int8_calib_cache);
            break;
        case PROP_OVERLAY_COLOR:
            g_value_set_uint(value, self->overlay_color);
            break;
        case PROP_PROFILE:
            g_value_set_boolean(value, self->profile);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static gboolean gst_deepdetect_start(GstBaseTransform *trans) {
    GstDeepDetect *self = GST_DEEPDETECT(trans);
    DeepDetectPrivate *priv = GST_DEEPDETECT_GET_PRIVATE(self);
    
    if (!self->engine_path) {
        GST_ERROR_OBJECT(self, "Engine path not set");
        return FALSE;
    }
    
    try {
        priv->engine = std::make_unique<YoloEngine>(self->engine_path, 
            self->int8_calib_cache);
        
        // Initialize CUDA context
        priv->cuda_context = gst_cuda_context_new(0); // Device 0
        if (!priv->cuda_context) {
            GST_ERROR_OBJECT(self, "Failed to create CUDA context");
            return FALSE;
        }
        
        cudaStreamCreate(&priv->stream);
        priv->initialized = TRUE;
        
        GST_INFO_OBJECT(self, "DeepDetect initialized successfully");
        return TRUE;
    } catch (const std::exception &e) {
        GST_ERROR_OBJECT(self, "Failed to initialize engine: %s", e.what());
        return FALSE;
    }
}

static gboolean gst_deepdetect_stop(GstBaseTransform *trans) {
    GstDeepDetect *self = GST_DEEPDETECT(trans);
    DeepDetectPrivate *priv = GST_DEEPDETECT_GET_PRIVATE(self);
    
    if (priv->initialized) {
        cudaStreamDestroy(priv->stream);
        gst_object_unref(priv->cuda_context);
        priv->engine.reset();
        priv->initialized = FALSE;
    }
    
    return TRUE;
}

static gboolean gst_deepdetect_set_caps(GstBaseTransform *trans,
    GstCaps *incaps, GstCaps *outcaps) {
    GstDeepDetect *self = GST_DEEPDETECT(trans);
    DeepDetectPrivate *priv = GST_DEEPDETECT_GET_PRIVATE(self);
    
    if (!gst_video_info_from_caps(&priv->video_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }
    
    GST_INFO_OBJECT(self, "Input: %dx%d, format: %s",
        GST_VIDEO_INFO_WIDTH(&priv->video_info),
        GST_VIDEO_INFO_HEIGHT(&priv->video_info),
        GST_VIDEO_INFO_NAME(&priv->video_info));
    
    return TRUE;
}

static GstFlowReturn gst_deepdetect_transform_ip(GstBaseTransform *trans,
    GstBuffer *buf) {
    GstDeepDetect *self = GST_DEEPDETECT(trans);
    DeepDetectPrivate *priv = GST_DEEPDETECT_GET_PRIVATE(self);
    
    if (!priv->initialized) {
        return GST_FLOW_ERROR;
    }
    
    // Start NVTX profiling if enabled
    if (self->profile) {
        nvtxRangePushA("DeepDetect::transform_ip");
    }
    
    GstMapInfo info;
    if (!gst_buffer_map(buf, &info, (GstMapFlags)(GST_MAP_READWRITE | GST_MAP_CUDA))) {
        GST_ERROR_OBJECT(self, "Failed to map CUDA buffer");
        return GST_FLOW_ERROR;
    }
    
    try {
        // Get CUDA stream from memory
        GstCudaMemory *cuda_mem = (GstCudaMemory*)info.memory;
        cudaStream_t stream = gst_cuda_memory_get_stream(cuda_mem);
        
        // Run inference
        std::vector<DetectionResult> detections = priv->engine->infer(
            info.data, 
            GST_VIDEO_INFO_WIDTH(&priv->video_info),
            GST_VIDEO_INFO_HEIGHT(&priv->video_info),
            stream
        );
        
        // Filter detections by score threshold
        std::vector<DetectionResult> filtered_detections;
        for (const auto &det : detections) {
            if (det.confidence >= self->score_threshold) {
                filtered_detections.push_back(det);
            }
        }
        
        // Create overlay composition
        if (!filtered_detections.empty()) {
            add_overlay_composition(buf, filtered_detections, self->overlay_color);
        }
        
        // Send metadata through src_meta pad
        send_detection_metadata(self, filtered_detections, GST_BUFFER_PTS(buf));
        
    } catch (const std::exception &e) {
        GST_ERROR_OBJECT(self, "Inference failed: %s", e.what());
        gst_buffer_unmap(buf, &info);
        return GST_FLOW_ERROR;
    }
    
    gst_buffer_unmap(buf, &info);
    
    if (self->profile) {
        nvtxRangePop();
    }
    
    return GST_FLOW_OK;
}

/* Helper function to send detection metadata */
void send_detection_metadata(GstDeepDetect *self, 
    const std::vector<DetectionResult> &detections, GstClockTime pts) {
    
    using json = nlohmann::json;
    json metadata;
    metadata["timestamp"] = pts;
    metadata["detections"] = json::array();
    
    for (const auto &det : detections) {
        json detection;
        detection["class_id"] = det.class_id;
        detection["confidence"] = det.confidence;
        detection["bbox"] = {det.x, det.y, det.w, det.h};
        metadata["detections"].push_back(detection);
    }
    
    std::string json_str = metadata.dump();
    
    GstBuffer *meta_buf = gst_buffer_new_allocate(NULL, json_str.size(), NULL);
    gst_buffer_fill(meta_buf, 0, json_str.c_str(), json_str.size());
    GST_BUFFER_PTS(meta_buf) = pts;
    
    gst_pad_push(self->src_meta, meta_buf);
}

/* Plugin registration */
static gboolean plugin_init(GstPlugin *plugin) {
    GST_DEBUG_CATEGORY_INIT(gst_deepdetect_debug, "deepdetect", 0,
        "GPU-accelerated YOLOv8 object detection");
    
    return gst_element_register(plugin, "deepdetect", GST_RANK_NONE,
        GST_TYPE_DEEPDETECT);
}

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    deepdetect,
    "GPU-accelerated YOLOv8 object detection using TensorRT",
    plugin_init,
    "1.0.0",
    "LGPL",
    "DeepDetect",
    "https://github.com/yourname/deepdetect-plugin"
)