project('deepdetect-plugin', 'cpp', 'cuda',
  version : '1.0.0',
  meson_version : '>= 0.60',
  default_options : [ 'cpp_std=c++17' ])

gst_dep = dependency('gstreamer-1.0', version : '>= 1.26')
gstbase_dep = dependency('gstreamer-base-1.0', version : '>= 1.26')
gstvideo_dep = dependency('gstreamer-video-1.0', version : '>= 1.26')
gstcuda_dep = dependency('gstreamer-cuda-1.0', version : '>= 1.26')

cuda_dep = dependency('cuda', version : '>= 12.4')
tensorrt_dep = dependency('tensorrt', version : '>= 10.0')

plugin_sources = [
  'src/gstdeepdetect.cc',
  'src/yolo_engine.cc',
]

gstdeepdetect = library('gstdeepdetect',
  plugin_sources,
  include_directories : include_directories('include'),
  dependencies : [gst_dep, gstbase_dep, gstvideo_dep, gstcuda_dep, cuda_dep, tensorrt_dep],
  install : true,
  install_dir : get_option('libdir') / 'gstreamer-1.0',
)