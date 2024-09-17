#include <gst/gst.h>  // GStreamer kütüphanesinin işlevlerini kullanmak için gerekli olan başlık dosyasını içerir


// Bu fonksiyon, medya dosyasının oynatılması ile ilgili tüm işlemleri gerçekleştirir.
int tutorial_main (int argc, char *argv[])
{
  GstElement *pipeline; // GStreamer elemanlarının bir araya getirildiği medya işleme zinciri (pipeline).
  GstBus *bus; // Pipeline’ın iletişim kurduğu mesaj otobüsü.
  GstMessage *msg; // Bus üzerinden alınan mesajlar, hata veya medya dosyasının sonlanması gibi durumlar hakkında bilgi içerir.


  /* Initialize GStreamer */
  // Bu, GStreamer kütüphanesini başlatır ve uygulamanın argümanlarını GStreamer ile entegre eder.
  gst_init (&argc, &argv);

  /* Build the pipeline */
  // GStreamer'ın bir medya dosyasını (bu örnekte bir web videosu) oynatabilmesi için kullanılan bir elementtir. Bu satır, bir web videosunu oynatmak üzere bir pipeline oluşturur.
  pipeline =
      gst_parse_launch
      ("playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm",
      NULL);

  /* Start playing */
  // Pipeline'ı çalıştırır, yani medya dosyasını oynatmaya başlar.
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait until error or EOS */
  // Bu satırlar, hata ya da EOS (End of Stream, akışın sonu) gibi mesajları bekler. 'gst_bus_timed_pop_filtered' fonksiyonu, belirtilen mesajları (hata veya EOS) almak için bekler.
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* See next tutorial for proper error message handling/parsing */
  // Eğer alınan mesaj bir hata mesajıysa, bu satır bir hata olduğunu kullanıcıya bildirir.
  if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
    g_printerr ("An error occurred! Re-run with the GST_DEBUG=*:WARN "
        "environment variable set for more details.\n");
  }

  /* Free resources */
  // Mesajı, bus'ı ve pipeline'ı temizleyerek kaynakları serbest bırakır. Pipeline'ı GST_STATE_NULL durumuna getirerek medya oynatmayı durdurur.
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
