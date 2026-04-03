#include <gst/gst.h>  // Includes the header file required to use GStreamer library functions


// This function handles all operations related to playing the media file.
int tutorial_main (int argc, char *argv[])
{
  GstElement *pipeline; // The media processing chain (pipeline) where GStreamer elements are assembled.
  GstBus *bus; // The message bus through which the pipeline communicates.
  GstMessage *msg; // Messages received from the bus, containing information about errors or end-of-stream events.


  /* Initialize GStreamer */
  // This initializes the GStreamer library and integrates the application’s arguments with GStreamer.
  gst_init (&argc, &argv);

  /* Build the pipeline */
  // This is an element used by GStreamer to play a media file (in this example, a web video). This line creates a pipeline to play a web video.
  pipeline =
      gst_parse_launch
      ("playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Start playing */
  // Starts the pipeline, i.e., begins playing the media file.
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  // These lines wait for messages such as errors or EOS (End of Stream). The ‘gst_bus_timed_pop_filtered’ function waits to receive the specified messages (error or EOS).
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* See next tutorial for proper error message handling/parsing */
  // If the received message is an error message, this line notifies the user that an error has occurred.
  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
    g_printerr ("An error occurred! Re-run with the GST_DEBUG=*:WARN "
        "environment variable set for more details.\n");
  }

  /* Free resources */
  // Frees resources by cleaning up the message, bus, and pipeline. Sets the pipeline to GST_STATE_NULL to stop media playback.
  gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  return 0;
}

// Tutorial main function runed
int main (int argc, char *argv[])
{
    return tutorial_main(argc, argv);
}
