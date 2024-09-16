import gi

gi.require_version("Gst", "1.0")
from gi.repository import Gst, GLib

# GST'yi başlatmak için gerekli olan argümanı ekleyin
Gst.init(None)

# Döngüyü başlatın
loop = GLib.MainLoop()

print("Starting loop...")
loop.run()