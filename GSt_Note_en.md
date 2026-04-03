**[English](GSt_Note_en.md) | [Turkce](GSt_Note_tr.md)**

# Gstreamer-Learning
I am currently learning Gstreamer with Cpp &amp; Python

 - run code => gcc basic-tutorial-1.c -o basic-tutorial-1 `pkg-config --cflags --libs gstreamer-1.0`




# GStreamer C/C++ Tutorial - English

<div style="text-align: center;">
  <div style="display: inline-block; margin-right: 20px;">
    <img src="./data/Gstreamer-logo.png" alt="GStreamer Logo" width="400" height="100"/>
  </div>
  <div style="display: inline-block;">
    <img src="./data/cpp-logo.png" alt="Cpp Logo" width="100" height="100"/>
  </div>
</div>


## Table of Contents

Note: This readme is actually used as a note, the project files should be evaluated separately!

- [1. Introduction](#1-introduction)
  - [1.1. What is GStreamer? Why Use It?](#11-what-is-gstreamer-why-use-it)
  - [1.2. Overview of Media Frameworks](#12-overview-of-media-frameworks)
  - [1.3. GStreamer's Core Philosophy: The Pipeline Approach](#13-gstreamers-core-philosophy-the-pipeline-approach)
- [2. Installation](#2-installation)
  - [2.1. Installation on Linux Systems](#21-installation-on-linux-systems)
  - [2.2. Installation on Windows Systems](#22-installation-on-windows-systems)
  - [2.3. Installation on macOS Systems](#23-installation-on-macos-systems)
- [3. Fundamental Concepts](#3-fundamental-concepts)
  - [3.1. Elements](#31-elements)
  - [3.2. Pads](#32-pads)
  - [3.3. Pipeline](#33-pipeline)
  - [3.4. Bins](#34-bins)
  - [3.5. Bus](#35-bus)
- [4. Creating a Simple Pipeline](#4-creating-a-simple-pipeline)
  - [4.1. Initializing the GStreamer Library](#41-initializing-the-gstreamer-library)
  - [4.2. Creating Elements](#42-creating-elements)
  - [4.3. Adding Elements to the Pipeline](#43-adding-elements-to-the-pipeline)
  - [4.4. Linking Elements](#44-linking-elements)
  - [4.5. Example: The Simplest Playback Application](#45-example-the-simplest-playback-application)
- [5. Pipeline States](#5-pipeline-states)
  - [5.1. State Types](#51-state-types)
  - [5.2. Changing States](#52-changing-states)
  - [5.3. Example: Managing State Changes](#53-example-managing-state-changes)
- [6. Bus and Messages](#6-bus-and-messages)
  - [6.1. Accessing the Bus](#61-accessing-the-bus)
  - [6.2. Pulling Messages](#62-pulling-messages)
  - [6.3. Message Types](#63-message-types)
  - [6.4. Example: Listening to and Processing Bus Messages](#64-example-listening-to-and-processing-bus-messages)
- [7. More Complex Pipelines](#7-more-complex-pipelines)
  - [7.1. Different Sources](#71-different-sources)
  - [7.2. Demuxers](#72-demuxers)
  - [7.3. Decoders](#73-decoders)
  - [7.4. Converters](#74-converters)
  - [7.5. Sinks](#75-sinks)
  - [7.6. Example: Playing a Video File](#76-example-playing-a-video-file)
  - [7.7. Example: Playing an Audio File](#77-example-playing-an-audio-file)
- [8. Error Handling](#8-error-handling)
  - [8.1. Checking Return Values](#81-checking-return-values)
  - [8.2. Catching Error Messages via the Bus](#82-catching-error-messages-via-the-bus)
  - [8.3. Example: Robust Error Handling](#83-example-robust-error-handling)
- [9. Events and Queries](#9-events-and-queries)
  - [9.1. The Role of Events](#91-the-role-of-events)
  - [9.2. The Role of Queries](#92-the-role-of-queries)
  - [9.3. Example: A Simple Seek Application](#93-example-a-simple-seek-application)
- [10. Practical Examples and Tips](#10-practical-examples-and-tips)
  - [10.1. Testing Pipelines with Command-line Tools](#101-testing-pipelines-with-command-line-tools)
  - [10.2. Useful Elements](#102-useful-elements)
  - [10.3. Multithreading and GStreamer](#103-multithreading-and-gstreamer)
- [11. Next Steps and Resources](#11-next-steps-and-resources)
  - [11.1. GStreamer Documentation](#111-gstreamer-documentation)
  - [11.2. GStreamer Plugins](#112-gstreamer-plugins)
  - [11.3. Advanced Topics](#113-advanced-topics)

## 1. Introduction

### 1.1. What is GStreamer? Why Use It?

GStreamer is a powerful and flexible open-source multimedia framework designed for developing multimedia applications. It provides a set of tools and libraries for processing audio, video, and other streaming data.

GStreamer's use cases:
- Media players
- Video/audio editing software
- Streaming systems
- Video conferencing applications
- Multimedia converters
- Media analysis tools

Key advantages of GStreamer:
- **Modular architecture**: Easy to add new formats and codecs
- **Platform independence**: Runs on Linux, Windows, macOS, iOS, Android, and other platforms
- **Performance focused**: Written in C, provides low latency
- **Rich plugin ecosystem**: Hundreds of ready-made components available
- **Flexible licensing**: Can be used in commercial projects under the LGPL license

### 1.2. Overview of Media Frameworks

Considering the complexity of media development, the importance of using a framework becomes immediately apparent. Media frameworks provide developers with abstractions for low-level media operations.

Common media frameworks:
- **GStreamer**: Cross-platform, general-purpose multimedia framework
- **FFmpeg**: Powerful command-line tools and libraries
- **DirectShow**: Microsoft's multimedia framework for Windows
- **Media Foundation**: The modern successor to DirectShow
- **AVFoundation**: Apple's multimedia framework for iOS and macOS

GStreamer stands out from these alternatives thanks to its cross-platform compatibility and modular pipeline-based architecture.

### 1.3. GStreamer's Core Philosophy: The Pipeline Approach

At the heart of GStreamer lies the **pipeline** concept. This approach is based on the Unix philosophy of "do one thing and do it well." Each media operation is performed by small, independent components designed for a specific task.

These components (elements) are connected along a pipeline, and data flows through this pipeline:

```
[Source] -> [Process1] -> [Process2] -> ... -> [Output]
```

For example, a simple video playback pipeline might look like this:

```
[File Source] -> [Demuxer] -> [Video Decoder] -> [Video Converter] -> [Video Display]
```

This modular approach provides the following advantages:
- **Reusability**: The same components can be used in different pipelines
- **Flexibility**: Components can be combined as desired
- **Extensibility**: New components can be easily added
- **Ease of maintenance**: Each component can be maintained independently

## 2. Installation

### 2.1. Installation on Linux Systems

On Debian/Ubuntu-based systems:

```bash
# Development libraries and tools
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

On Fedora/Red Hat-based systems:

```bash
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-plugins-good gstreamer1-plugins-good-extras gstreamer1-plugins-bad-free gstreamer1-plugins-bad-free-devel gstreamer1-plugins-bad-free-extras
```

Arch Linux:

```bash
sudo pacman -S gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav
```

### 2.2. Installation on Windows Systems

There are two main methods for using GStreamer on Windows:

1. **Using MSYS2/MinGW**:
   ```bash
   pacman -S mingw-w64-x86_64-gstreamer mingw-w64-x86_64-gst-plugins-base mingw-w64-x86_64-gst-plugins-good mingw-w64-x86_64-gst-plugins-bad mingw-w64-x86_64-gst-plugins-ugly mingw-w64-x86_64-gst-libav
   ```

2. **Official Windows Installers**:
   - [GStreamer download page](https://gstreamer.freedesktop.org/download/)
   - Download both development and runtime packages
   - After downloading, add the GStreamer bin directory to the PATH environment variable

You can use [vcpkg](https://github.com/microsoft/vcpkg) for pkgconfig support when using with Visual Studio.

### 2.3. Installation on macOS Systems

Installation on macOS using Homebrew:

```bash
brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav
```

## 3. Fundamental Concepts

### 3.1. Elements

**Elements** are the fundamental building blocks of GStreamer. Each element performs a specific task and represents a processing point within the pipeline.

Element types:
- **Source Elements**: Elements that produce data (e.g., `filesrc`, `videotestsrc`)
- **Filter Elements**: Elements that modify data (e.g., `videoconvert`, `audioresample`)
- **Sink Elements**: Elements that consume data (e.g., `autovideosink`, `filesink`)

Each element is an independent component on its own and has its own internal state management.

```c
// Example of creating an element
GstElement *source = gst_element_factory_make("videotestsrc", "source");
GstElement *sink = gst_element_factory_make("autovideosink", "sink");

// Setting element properties
g_object_set(G_OBJECT(source), "pattern", 0, NULL);  // Test pattern 0 (SMPTE)
```

### 3.2. Pads

**Pads** are the connection points of elements. Through pads, elements are connected to each other to enable data flow.

Pad types:
- **Source Pad**: The point where data flows out of the element
- **Sink Pad**: The point where data flows into the element

Each pad has a specific **capability** (abbreviated as "caps") set. These capabilities define what data types the pad can handle.

```
[Element A] --source pad--> sink pad-->[Element B]
```

```c
// Manually linking two elements through pads
GstPad *src_pad = gst_element_get_static_pad(element_a, "src");
GstPad *sink_pad = gst_element_get_static_pad(element_b, "sink");
gst_pad_link(src_pad, sink_pad);
gst_object_unref(src_pad);
gst_object_unref(sink_pad);
```

### 3.3. Pipeline

A **Pipeline** is a special container that brings elements together and synchronizes them. Pipelines provide:
- Coordination of data flow between elements
- Global clock management
- Pipeline state management
- Media position and duration tracking

A Pipeline is actually a special type of Bin.

```c
// Creating a pipeline
GstElement *pipeline = gst_pipeline_new("test-pipeline");

// Adding elements to the pipeline
gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);

// Changing the pipeline state
gst_element_set_state(pipeline, GST_STATE_PLAYING);
```

### 3.4. Bins

**Bins** are containers that can group elements. A Bin manages the elements within it as a single element, allowing you to build complex pipelines in a modular way.

Benefits of Bins:
- Managing complexity
- Creating reusable element groups
- Building hierarchical pipelines

```c
// Creating a bin
GstElement *bin = gst_bin_new("my-bin");

// Adding elements to the bin
gst_bin_add_many(GST_BIN(bin), decoder, converter, NULL);

// Adding the bin to the pipeline
gst_bin_add(GST_BIN(pipeline), bin);
```

### 3.5. Bus

The **Bus** is a communication system through which asynchronous messages from elements within the pipeline are transmitted. The Bus carries messages from elements to the application's main loop.

Message types that can be received via the Bus:
- Error messages
- End-of-Stream (EOS) messages
- State change messages
- Tag messages (metadata)
- Warning messages

```c
// Getting the pipeline's bus
GstBus *bus = gst_element_get_bus(pipeline);

// Non-blocking wait for messages (polling)
GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
    GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);

// Message processing
// ...

// Cleanup
gst_message_unref(msg);
gst_object_unref(bus);
```

## 4. Creating a Simple Pipeline

### 4.1. Initializing the GStreamer Library

Before writing a GStreamer application, the library must be initialized. This handles processing command-line arguments, loading plugins, and preparing GStreamer for use.

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    // GStreamer code...
    
    // When exiting the program (optional)
    gst_deinit();
    
    return 0;
}
```

### 4.2. Creating Elements

Elements are created using the `gst_element_factory_make()` function. This function takes two parameters:
1. The name of the element factory (which element type)
2. A unique name to give to the element (can be NULL)

```c
// Creating a source element
GstElement *source = gst_element_factory_make("videotestsrc", "test-source");
if (!source) {
    g_printerr("Could not create videotestsrc element\n");
    return -1;
}

// Creating a sink element
GstElement *sink = gst_element_factory_make("autovideosink", "test-sink");
if (!sink) {
    g_printerr("Could not create autovideosink element\n");
    return -1;
}
```

### 4.3. Adding Elements to the Pipeline

To add created elements to the pipeline, use the `gst_bin_add()` or `gst_bin_add_many()` functions.

```c
// Creating a pipeline
GstElement *pipeline = gst_pipeline_new("test-pipeline");

// Adding a single element to the pipeline
gst_bin_add(GST_BIN(pipeline), source);

// Adding multiple elements to the pipeline
gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);
```

### 4.4. Linking Elements

To link elements together, use the `gst_element_link()` or `gst_element_link_many()` functions.

```c
// Linking two elements
if (!gst_element_link(source, sink)) {
    g_printerr("Elements could not be linked\n");
    gst_object_unref(pipeline);
    return -1;
}

// Linking multiple elements in sequence
if (!gst_element_link_many(source, filter1, filter2, sink, NULL)) {
    g_printerr("Elements could not be linked\n");
    gst_object_unref(pipeline);
    return -1;
}
```

### 4.5. Example: The Simplest Playback Application

Below is a simple GStreamer application that generates a test video pattern and displays it on screen:

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Create elements */
    source = gst_element_factory_make("videotestsrc", "source");
    sink = gst_element_factory_make("autovideosink", "sink");
    
    /* Check that elements were created successfully */
    if (!source || !sink) {
        g_printerr("One of the elements could not be created. Make sure GStreamer plugins are properly installed.\n");
        return -1;
    }
    
    /* Example of setting a property */
    g_object_set(G_OBJECT(source), "pattern", 0, NULL);  // SMPTE test pattern
    
    /* Create an empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");
    
    /* Check that the pipeline was created successfully */
    if (!pipeline) {
        g_printerr("Pipeline could not be created.\n");
        return -1;
    }
    
    /* Add elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    
    /* Link elements */
    if (!gst_element_link(source, sink)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Set pipeline state to PLAYING */
    g_print("Starting pipeline...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Get the pipeline's bus */
    bus = gst_element_get_bus(pipeline);
    
    /* Wait for ERROR or EOS messages */
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Analyze message and perform appropriate action */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error received: %s\n", err->message);
                g_printerr("Debug info: %s\n", debug_info ? debug_info : "None");
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream received.\n");
                break;
            default:
                g_printerr("Unexpected message received.\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Stop the pipeline */
    g_print("Stopping pipeline...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Clean up resources */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}
```

Compilation:

```bash
gcc -o basic-player basic-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

## 5. Pipeline States

### 5.1. State Types

GStreamer pipelines and elements can be in four basic states:

1. **GST_STATE_NULL**: Default initial state. No resources are allocated, the element is not prepared.
2. **GST_STATE_READY**: Element is initialized but data is not flowing. Resources are allocated but buffer memory may not be allocated.
3. **GST_STATE_PAUSED**: Element is initialized, data is being processed but the flow is paused (at most one buffer has been processed).
4. **GST_STATE_PLAYING**: Element is fully active, data is flowing at normal speed.

Transitions between states must follow this order:
```
NULL <-> READY <-> PAUSED <-> PLAYING
```

Each state change triggers specific preparation and resource allocation/release operations.

### 5.2. Changing States

To change the state of a pipeline or element, use the `gst_element_set_state()` function:

```c
GstStateChangeReturn ret;

// Set pipeline to PLAYING state
ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

// Check the state change result
switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
        g_printerr("State change failed!\n");
        // Error handling...
        break;
    case GST_STATE_CHANGE_SUCCESS:
        g_print("State changed successfully\n");
        break;
    case GST_STATE_CHANGE_ASYNC:
        g_print("State change is continuing asynchronously\n");
        // If desired, we can wait for the state change to complete
        ret = gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        g_print("Live source, preroll will not occur\n");
        break;
}
```

### 5.3. Example: Managing State Changes

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *sink;
    GstStateChangeReturn ret;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Create elements */
    source = gst_element_factory_make("videotestsrc", "source");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("test-pipeline");
    
    /* Checks and pipeline setup... */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    gst_element_link(source, sink);
    
    /* Transition to READY state */
    g_print("Setting pipeline to READY state...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to READY state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Get the pipeline's current state */
    GstState current, pending;
    ret = gst_element_get_state(pipeline, &current, &pending, GST_CLOCK_TIME_NONE);
    g_print("Current state: %s, Pending state: %s\n", 
            gst_element_state_get_name(current), 
            gst_element_state_get_name(pending));
    
    /* Transition to PAUSED state */
    g_print("Setting pipeline to PAUSED state...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to PAUSED state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Wait for transition to PAUSED state to complete */
    ret = gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    g_print("Transition to PAUSED state completed\n");
    
    /* Transition to PLAYING state */
    g_print("Setting pipeline to PLAYING state...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Wait for 5 seconds of playback */
    g_print("Playing for 5 seconds...\n");
    g_usleep(5 * 1000000);
    
    /* Transition to PAUSED state (pause) */
    g_print("Pausing pipeline (PAUSED state)...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    
    /* Wait 2 seconds */
    g_usleep(2 * 1000000);
    
    /* Transition to PLAYING state (resume) */
    g_print("Resuming pipeline (PLAYING state)...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    /* Wait for 3 more seconds of playback */
    g_usleep(3 * 1000000);
    
    /* Transition to NULL state (stop completely and release resources) */
    g_print("Stopping pipeline (NULL state)...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Clean up resources */
    gst_object_unref(pipeline);
    
    return 0;
}
```

---


## 6. Bus and Messages

### 6.1. Accessing the Bus

The GStreamer bus is a mechanism that carries messages from elements to the application's main loop. To access the bus:

```c
// Getting the pipeline's bus
GstBus *bus = gst_element_get_bus(pipeline);
```

### 6.2. Pulling Messages

There are several methods for receiving messages from the bus:

1. **Blocking Message Retrieval**:
   ```c
   // Wait indefinitely and get messages of specific types
   GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
       GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   ```

2. **Non-blocking Message Check**:
   ```c
   // Get the message immediately if available, otherwise return NULL
   GstMessage *msg = gst_bus_pop_filtered(bus,
       GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   ```

3. **Timed Wait**:
   ```c
   // Wait at most 100ms
   GstMessage *msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
       GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   ```

### 6.3. Message Types

Most common message types:

1. **GST_MESSAGE_ERROR**: Sent when an error occurs
2. **GST_MESSAGE_EOS**: End-of-Stream, sent when the end of the stream is reached
3. **GST_MESSAGE_STATE_CHANGED**: Sent when an element's state changes
4. **GST_MESSAGE_TAG**: Sent when media metadata (tags) is found
5. **GST_MESSAGE_BUFFERING**: Sent when the buffering status changes
6. **GST_MESSAGE_CLOCK_LOST**: Sent when the pipeline clock is lost
7. **GST_MESSAGE_NEW_CLOCK**: Sent when a new clock is selected
8. **GST_MESSAGE_DURATION_CHANGED**: Sent when the media duration changes
9. **GST_MESSAGE_ASYNC_DONE**: Sent when an asynchronous state change is completed

### 6.4. Example: Listening to and Processing Bus Messages

```c
#include <gst/gst.h>

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *) data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;
            
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            
            g_main_loop_quit(loop);
            break;
        }
        
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            
            // Only track pipeline state changes
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data)) {
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                g_print("Pipeline state change: %s -> %s\n",
                        gst_element_state_get_name(old_state),
                        gst_element_state_get_name(new_state));
            }
            break;
        }
        
        case GST_MESSAGE_TAG: {
            GstTagList *tags = NULL;
            
            gst_message_parse_tag(msg, &tags);
            
            g_print("New tags received:\n");
            gst_tag_list_foreach(tags, print_tag, NULL);
            
            gst_tag_list_unref(tags);
            break;
        }
        
        default:
            break;
    }
    
    // Return TRUE to continue processing signals
    return TRUE;
}

static void print_tag(const GstTagList *tags, const gchar *tag, gpointer user_data) {
    GValue val = { 0, };
    gchar *str;
    gint count;
    
    count = gst_tag_list_get_tag_size(tags, tag);
    
    for (gint i = 0; i < count; i++) {
        gst_tag_list_index(tags, tag, i, &val);
        
        if (G_VALUE_HOLDS_STRING(&val))
            str = g_value_dup_string(&val);
        else
            str = gst_value_serialize(&val);
        
        g_print("  %s: %s\n", tag, str);
        g_free(str);
        
        g_value_unset(&val);
    }
}

int main(int argc, char *argv[]) {
    GMainLoop *loop;
    GstElement *pipeline, *source, *decoder, *sink;
    GstBus *bus;
    guint bus_watch_id;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* A file name is required for checking */
    if (argc != 2) {
        g_printerr("Specify a media file: %s file_name\n", argv[0]);
        return -1;
    }
    
    /* Create main loop */
    loop = g_main_loop_new(NULL, FALSE);
    
    /* Create pipeline elements */
    pipeline = gst_pipeline_new("audio-player");
    source = gst_element_factory_make("filesrc", "file-source");
    decoder = gst_element_factory_make("decodebin", "decoder");
    sink = gst_element_factory_make("autoaudiosink", "audio-sink");
    
    if (!pipeline || !source || !decoder || !sink) {
        g_printerr("One of the elements could not be created.\n");
        return -1;
    }
    
    /* Set the file name to the source */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Add bus watcher */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);
    
    /* Set up the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, NULL);
    
    /* Link source and decoder */
    gst_element_link(source, decoder);
    
    /* Add pad-added signal handler for the decoder */
    g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), sink);
    
    /* Set pipeline to PLAYING state */
    g_print("Starting pipeline...\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    /* Run the main loop */
    g_print("Main loop running...\n");
    g_main_loop_run(loop);
    
    /* Clean up */
    g_print("Main loop terminated, cleaning up resources...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    
    return 0;
}

/* Function called when decodebin creates a pad */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *sink = (GstElement *) data;
    GstPad *sinkpad;
    
    /* Get the sink's sink pad */
    sinkpad = gst_element_get_static_pad(sink, "sink");
    
    /* If the sink pad is already linked, do nothing */
    if (gst_pad_is_linked(sinkpad)) {
        gst_object_unref(sinkpad);
        return;
    }
    
    /* Link the pad from decoder to the sink pad */
    GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
    
    if (GST_PAD_LINK_FAILED(ret)) {
        g_print("Pad link failed\n");
    } else {
        g_print("Pad link successful\n");
    }
    
    gst_object_unref(sinkpad);
}
```

Compilation:

```bash
gcc -o bus-example bus-example.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Running:

```bash
./bus-example media_file.mp3
```


---


## 7. More Complex Pipelines

### 7.1. Different Sources

GStreamer has many source elements:

1. **filesrc**: Element that reads data from a file
   ```c
   GstElement *source = gst_element_factory_make("filesrc", "file-source");
   g_object_set(G_OBJECT(source), "location", "video.mp4", NULL);
   ```

2. **uridecodebin**: Element that reads data from a URI and performs demux/decode
   ```c
   GstElement *source = gst_element_factory_make("uridecodebin", "uri-source");
   g_object_set(G_OBJECT(source), "uri", "https://example.com/video.mp4", NULL);
   ```

3. **videotestsrc**: Element that generates test video patterns
   ```c
   GstElement *source = gst_element_factory_make("videotestsrc", "test-source");
   g_object_set(G_OBJECT(source), "pattern", 0, NULL); // SMPTE pattern
   ```

4. **audiotestsrc**: Element that generates test audio signals
   ```c
   GstElement *source = gst_element_factory_make("audiotestsrc", "test-source");
   g_object_set(G_OBJECT(source), "freq", 440.0, NULL); // 440 Hz sine wave (A note)
   ```

5. **v4l2src**: Source for Video4Linux2 compatible cameras
   ```c
   GstElement *source = gst_element_factory_make("v4l2src", "camera-source");
   g_object_set(G_OBJECT(source), "device", "/dev/video0", NULL);
   ```

### 7.2. Demuxers

Demuxers are elements that separate audio and video streams from container formats (MP4, MKV, AVI, etc.).

Popular demuxers:
- **qtdemux**: For MP4/MOV files
- **matroskademux**: For MKV files
- **oggdemux**: For OGG files
- **flvdemux**: For FLV files

A key feature of demuxers is that they create dynamic pads. A new src pad is created each time a stream is found. Therefore, we need to use signal handlers to link demuxers to other elements:

```c
// Creating a demuxer
GstElement *demuxer = gst_element_factory_make("qtdemux", "demuxer");

// Adding a pad-added signal handler to the demuxer
g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), NULL);

// Signal handler
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    // Get the pad's capabilities
    GstCaps *caps = gst_pad_get_current_caps(pad);
    GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);
    
    // Handle video stream
    if (g_str_has_prefix(name, "video/")) {
        // Link to video decoder
        // ...
    }
    // Handle audio stream
    else if (g_str_has_prefix(name, "audio/")) {
        // Link to audio decoder
        // ...
    }
    
    gst_caps_unref(caps);
}
```

### 7.3. Decoders

Decoders are elements that convert compressed media data (H.264, VP8, MP3, AAC, etc.) into processable raw formats.

Popular video decoders:
- **avdec_h264**: H.264 video decoder
- **avdec_mpeg2video**: MPEG-2 video decoder
- **vp8dec**: VP8 video decoder
- **vp9dec**: VP9 video decoder

Popular audio decoders:
- **avdec_mp3**: MP3 audio decoder
- **avdec_aac**: AAC audio decoder
- **vorbisdec**: Vorbis audio decoder
- **opusdec**: Opus audio decoder

Generally, instead of directly using a specific codec, it is easier to use the **decodebin** element which can perform automatic codec selection:

```c
// Decoder with automatic codec selection
GstElement *decoder = gst_element_factory_make("decodebin", "decoder");

// Adding a signal handler to the decoder
g_signal_connect(decoder, "pad-added", G_CALLBACK(on_decoder_pad_added), NULL);
```

### 7.4. Converters

Converters are elements that transform one media format into another. Format conversions ensure that different elements work compatibly.

Popular converters:
- **videoconvert**: Video pixel format converter
- **audioconvert**: Audio format converter
- **audioresample**: Audio sample rate converter
- **videoscale**: Video size scaler
- **videorate**: Video frame rate converter

```c
// Video converter
GstElement *converter = gst_element_factory_make("videoconvert", "converter");

// Audio converter and sample rate changer
GstElement *audioconvert = gst_element_factory_make("audioconvert", "aconv");
GstElement *audioresample = gst_element_factory_make("audioresample", "aresample");
```

### 7.5. Sinks

Sinks are the final elements of the pipeline and perform tasks that consume data (displaying, saving, sending over the network, etc.).

Popular sinks:
- **autovideosink**: Appropriate video display for the system
- **autoaudiosink**: Appropriate audio output for the system
- **filesink**: Data recorder to file
- **udpsink**: Data sender over UDP
- **rtmpsink**: Broadcaster over RTMP

```c
// Video display
GstElement *videosink = gst_element_factory_make("autovideosink", "video-sink");

// Audio output
GstElement *audiosink = gst_element_factory_make("autoaudiosink", "audio-sink");

// File recording
GstElement *filesink = gst_element_factory_make("filesink", "file-sink");
g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);
```

### 7.6. Example: Playing a Video File

The following example creates a more complex pipeline to play a video file:

```c
#include <gst/gst.h>

/* Signal handler function prototype */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *demuxer, *video_decoder, *video_convert, *video_sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Make sure a media file is specified */
    if (argc != 2) {
        g_printerr("Specify a video file: %s <file>\n", argv[0]);
        return -1;
    }
    
    /* Create elements */
    pipeline = gst_pipeline_new("video-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demuxer = gst_element_factory_make("qtdemux", "demuxer");      // MP4/MOV demuxer
    video_decoder = gst_element_factory_make("avdec_h264", "video-decoder");  // H.264 decoder
    video_convert = gst_element_factory_make("videoconvert", "converter");
    video_sink = gst_element_factory_make("autovideosink", "video-sink");
    
    /* Make sure all elements were created */
    if (!pipeline || !source || !demuxer || !video_decoder || !video_convert || !video_sink) {
        g_printerr("One of the elements could not be created.\n");
        return -1;
    }
    
    /* Set the file location */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Add elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, video_decoder, video_convert, video_sink, NULL);
    
    /* Start linking elements */
    /* source -> demuxer */
    if (!gst_element_link(source, demuxer)) {
        g_printerr("Elements could not be linked: source -> demuxer\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Add signal watcher for demuxer's h264 video stream */
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), video_decoder);
    
    /* Link video stream to subsequent elements */
    if (!gst_element_link_many(video_decoder, video_convert, video_sink, NULL)) {
        g_printerr("Elements could not be linked: decoder -> converter -> sink\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Set pipeline state to PLAYING */
    g_print("Starting pipeline...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Wait for messages on the bus */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Message processing */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error %s\n", err->message);
                if (debug_info) {
                    g_printerr("Debug info: %s\n", debug_info);
                }
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                break;
            default:
                g_printerr("Unexpected message received\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Stop the pipeline */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Clean up resources */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}

/* Function called when the demuxer creates a pad */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *decoder = (GstElement *) data;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    const gchar *name;
    
    /* Get the pad's capabilities */
    caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    structure = gst_caps_get_structure(caps, 0);
    name = gst_structure_get_name(structure);
    
    /* We are only interested in video streams */
    if (g_str_has_prefix(name, "video/x-h264")) {
        /* Get the decoder's sink pad */
        sink_pad = gst_element_get_static_pad(decoder, "sink");
        
        /* Link the pads */
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sink_pad))) {
            g_printerr("Pad link from demuxer to video decoder failed.\n");
        } else {
            g_print("Demuxer -> video decoder pad link successful.\n");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
}
```

Compilation:

```bash
gcc -o video-player video-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Running:

```bash
./video-player video.mp4
```

### 7.7. Example: Playing an Audio File

The following example creates a pipeline to play an audio file:

```c
#include <gst/gst.h>

/* Signal handler function prototype */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *demuxer, *audio_decoder, *audio_convert, *audio_resample, *audio_sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Make sure a media file is specified */
    if (argc != 2) {
        g_printerr("Specify an audio file: %s <file>\n", argv[0]);
        return -1;
    }
    
    /* Create elements */
    pipeline = gst_pipeline_new("audio-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demuxer = gst_element_factory_make("oggdemux", "demuxer");       // OGG demuxer
    audio_decoder = gst_element_factory_make("vorbisdec", "audio-decoder");  // Vorbis decoder
    audio_convert = gst_element_factory_make("audioconvert", "converter");
    audio_resample = gst_element_factory_make("audioresample", "resampler");
    audio_sink = gst_element_factory_make("autoaudiosink", "audio-sink");
    
    /* Make sure all elements were created */
    if (!pipeline || !source || !demuxer || !audio_decoder || 
        !audio_convert || !audio_resample || !audio_sink) {
        g_printerr("One of the elements could not be created.\n");
        return -1;
    }
    
    /* Set the file location */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Add elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), 
                    source, demuxer, audio_decoder, 
                    audio_convert, audio_resample, audio_sink, NULL);
    
    /* Start linking elements */
    /* source -> demuxer */
    if (!gst_element_link(source, demuxer)) {
        g_printerr("Elements could not be linked: source -> demuxer\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Add signal watcher for demuxer's vorbis audio stream */
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), audio_decoder);
    
    /* Link audio stream to subsequent elements */
    if (!gst_element_link_many(audio_decoder, audio_convert, audio_resample, audio_sink, NULL)) {
        g_printerr("Elements could not be linked: decoder -> converter -> resampler -> sink\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Set pipeline state to PLAYING */
    g_print("Starting pipeline...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Wait for messages on the bus */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Message processing */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error %s\n", err->message);
                if (debug_info) {
                    g_printerr("Debug info: %s\n", debug_info);
                }
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                break;
            default:
                g_printerr("Unexpected message received\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Stop the pipeline */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Clean up resources */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}

/* Function called when the demuxer creates a pad */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *decoder = (GstElement *) data;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    const gchar *name;
    
    /* Get the pad's capabilities */
    caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    structure = gst_caps_get_structure(caps, 0);
    name = gst_structure_get_name(structure);
    
    /* We are only interested in audio streams */
    if (g_str_has_prefix(name, "audio/x-vorbis")) {
        /* Get the decoder's sink pad */
        sink_pad = gst_element_get_static_pad(decoder, "sink");
        
        /* Link the pads */
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sink_pad))) {
            g_printerr("Pad link from demuxer to audio decoder failed.\n");
        } else {
            g_print("Demuxer -> audio decoder pad link successful.\n");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
}
```

Compilation:

```bash
gcc -o audio-player audio-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Running:

```bash
./audio-player music.ogg
```

---


## 8. Error Handling

### 8.1. Checking Return Values

Most GStreamer API functions return values indicating success or failure. Checking these values is the best way to detect errors early:

```c
// Creating an element
GstElement *element = gst_element_factory_make("elementname", "custom-name");
if (!element) {
    g_printerr("Could not create element. Is the element plugin installed?\n");
    // Error handling...
    return -1;
}

// Linking elements
if (!gst_element_link(element1, element2)) {
    g_printerr("Elements could not be linked. Could the pads be incompatible?\n");
    // Error handling...
    return -1;
}

// Changing state
GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Pipeline state could not be changed. There may be an issue with the elements.\n");
    // Error handling...
    return -1;
}
```

### 8.2. Catching Error Messages via the Bus

To catch errors that occur while the pipeline is running, you need to listen to bus messages:

```c
// Wait for error or EOS messages on the bus
GstBus *bus = gst_element_get_bus(pipeline);
GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
    GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

// Process the message
if (msg != NULL) {
    GError *err;
    gchar *debug_info;
    
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Pipeline error: %s\n", err->message);
        g_printerr("Debug info: %s\n", debug_info ? debug_info : "None");
        g_free(debug_info);
        g_error_free(err);
    }
    
    gst_message_unref(msg);
}
```

### 8.3. Example: Robust Error Handling

The following example demonstrates an application with comprehensive error handling:

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline = NULL;
    GstElement *source = NULL, *converter = NULL, *sink = NULL;
    GstBus *bus = NULL;
    GstMessage *msg = NULL;
    GstStateChangeReturn state_ret;
    gboolean terminate = FALSE;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Define cleanup points for error handling */
    do {
        /* Create pipeline elements */
        pipeline = gst_pipeline_new("error-handling-example");
        if (!pipeline) {
            g_printerr("Could not create pipeline\n");
            break;
        }
        
        source = gst_element_factory_make("videotestsrc", "source");
        if (!source) {
            g_printerr("Could not create videotestsrc. Are GStreamer plugins properly installed?\n");
            break;
        }
        
        converter = gst_element_factory_make("videoconvert", "converter");
        if (!converter) {
            g_printerr("Could not create videoconvert\n");
            break;
        }
        
        sink = gst_element_factory_make("autovideosink", "sink");
        if (!sink) {
            g_printerr("Could not create autovideosink\n");
            break;
        }
        
        /* Add elements to the pipeline */
        gst_bin_add_many(GST_BIN(pipeline), source, converter, sink, NULL);
        
        /* Link elements */
        if (!gst_element_link_many(source, converter, sink, NULL)) {
            g_printerr("Elements could not be linked\n");
            break;
        }
        
        /* Set pipeline state to PLAYING */
        g_print("Starting pipeline...\n");
        state_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (state_ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("Pipeline could not be set to PLAYING state\n");
            break;
        }
        
        /* Get the bus and listen for messages */
        bus = gst_element_get_bus(pipeline);
        
        /* Wait for asynchronous state change to complete */
        if (state_ret == GST_STATE_CHANGE_ASYNC) {
            g_print("Waiting for asynchronous state change...\n");
            state_ret = gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
            if (state_ret == GST_STATE_CHANGE_FAILURE) {
                g_printerr("State change failed\n");
                break;
            }
        }
        
        g_print("Pipeline started, will run for 5 seconds...\n");
        
        /* Message loop */
        terminate = FALSE;
        do {
            msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);
            
            /* Timeout (NULL message) - normal exit */
            if (!msg) {
                g_print("5 second timeout reached, exiting...\n");
                terminate = TRUE;
                break;
            }
            
            /* Process message */
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug;
                    
                    gst_message_parse_error(msg, &err, &debug);
                    g_printerr("Error: %s\n", err->message);
                    if (debug) {
                        g_printerr("Debug Details: %s\n", debug);
                    }
                    g_error_free(err);
                    g_free(debug);
                    
                    terminate = TRUE;
                    break;
                }
                case GST_MESSAGE_EOS:
                    g_print("End of Stream received\n");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    /* Only track pipeline state changes */
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                        g_print("Pipeline state change: %s -> %s\n",
                                gst_element_state_get_name(old_state),
                                gst_element_state_get_name(new_state));
                    }
                    break;
                default:
                    g_printerr("Unexpected message received\n");
                    break;
            }
            
            gst_message_unref(msg);
            
        } while (!terminate);
        
    } while (FALSE); /* This do-while block runs only once, used for early exit */
    
    /* Cleanup */
    g_print("Cleaning up resources...\n");
    
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    
    if (bus) {
        gst_object_unref(bus);
    }
    
    return 0;
}
```

Compilation:

```bash
gcc -o error-handling error-handling.c $(pkg-config --cflags --libs gstreamer-1.0)
```

## 9. Events and Queries

### 9.1. The Role of Events

**Events** are used to change element states or stream behavior within the pipeline. Events generally travel in the reverse direction of the stream (upstream or downstream).

Common event types:
- **Seek Events**: For changing the media position
- **Flush Events**: For flushing buffers
- **EOS Events**: For sending End-of-Stream signals
- **QOS Events**: For sending Quality of Service information

```c
// Example of sending a seek event (jumping to a specific position in the media)
gboolean seek_res;
seek_res = gst_element_seek_simple(
    pipeline,                  // Element to perform the operation
    GST_FORMAT_TIME,           // Position format (time)
    GST_SEEK_FLAG_FLUSH |      // Flush buffers
    GST_SEEK_FLAG_KEY_UNIT,    // Stop at the nearest key frame
    30 * GST_SECOND            // Jump to the 30th second
);

if (!seek_res) {
    g_printerr("Seek operation failed\n");
}
```

### 9.2. The Role of Queries

**Queries** are used to retrieve information from elements. You can send various information questions to elements within the pipeline.

Common query types:
- **Position Query**: Current position information
- **Duration Query**: Media duration
- **Seeking Query**: Seeking support and range
- **Formats Query**: Supported format types

```c
// Example of querying media duration
gint64 duration;
gboolean res;

res = gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration);
if (res) {
    g_print("Media duration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(duration));
} else {
    g_printerr("Could not query duration\n");
}

// Example of querying current position
gint64 position;
res = gst_element_query_position(pipeline, GST_FORMAT_TIME, &position);
if (res) {
    g_print("Current position: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(position));
} else {
    g_printerr("Could not query position\n");
}
```

### 9.3. Example: A Simple Seek Application

The following example is a simple application that plays a media file and allows the user to seek forward/backward in the media:

```c
#include <gst/gst.h>
#include <stdio.h>

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, gpointer data);
static gboolean seek_element(GstElement *pipeline, gdouble rate, gint64 position);

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GIOChannel *io_stdin;
    guint io_watch_id;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Usage information */
    if (argc != 2) {
        g_printerr("Usage: %s <media file>\n", argv[0]);
        return -1;
    }
    
    /* Create playbin element (sets up automatic pipeline) */
    pipeline = gst_element_factory_make("playbin", "player");
    if (!pipeline) {
        g_printerr("Could not create playbin element\n");
        return -1;
    }
    
    /* Set the file to play */
    g_object_set(G_OBJECT(pipeline), "uri", gst_filename_to_uri(argv[1], NULL), NULL);
    
    /* Set up keyboard input channel */
    io_stdin = g_io_channel_unix_new(fileno(stdin));
    g_io_channel_set_flags(io_stdin, G_IO_FLAG_NONBLOCK, NULL);
    
    /* Display usage information */
    g_print(
        "\n"
        "*************************************************\n"
        "* 'q': Quit                                     *\n"
        "* 'p': Play/Pause                               *\n"
        "* '+': 10 seconds forward                       *\n"
        "* '-': 10 seconds backward                      *\n"
        "* 'r': Return to normal speed (1.0x)            *\n"
        "* '1': 0.5x speed (slow)                        *\n"
        "* '2': 2.0x speed (fast)                        *\n"
        "*************************************************\n\n"
    );
    
    /* Add keyboard watcher */
    io_watch_id = g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, pipeline);
    
    /* Play the pipeline */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    /* Start main loop */
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    
    /* Cleanup */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(io_watch_id);
    g_io_channel_unref(io_stdin);
    g_main_loop_unref(loop);
    
    return 0;
}

/* Handle keyboard keys */
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, gpointer data) {
    GstElement *pipeline = (GstElement *)data;
    GstState state, pending;
    gchar *str = NULL;
    
    if (g_io_channel_read_line(source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) {
        return TRUE;
    }
    
    switch (str[0]) {
        case 'q':
        case 'Q':
            g_main_loop_quit((GMainLoop *)g_main_loop_new(NULL, FALSE));
            break;
        case 'p':
        case 'P':
            gst_element_get_state(pipeline, &state, &pending, 0);
            if (state == GST_STATE_PLAYING)
                gst_element_set_state(pipeline, GST_STATE_PAUSED);
            else
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
            break;
        case '+':
            {
                gint64 position;
                if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
                    seek_element(pipeline, 1.0, position + 10 * GST_SECOND);
                }
            }
            break;
        case '-':
            {
                gint64 position;
                if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
                    seek_element(pipeline, 1.0, position - 10 * GST_SECOND);
                }
            }
            break;
        case 'r':
        case 'R':
            seek_element(pipeline, 1.0, -1);
            break;
        case '1':
            seek_element(pipeline, 0.5, -1);
            break;
        case '2':
            seek_element(pipeline, 2.0, -1);
            break;
        default:
            break;
    }
    
    g_free(str);
    return TRUE;
}

/* Perform the seek operation */
static gboolean seek_element(GstElement *pipeline, gdouble rate, gint64 position) {
    GstEvent *seek_event;
    
    /* Keep current position (if -1) */
    if (position < 0) {
        if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
            g_printerr("Could not get current position\n");
            return FALSE;
        }
    }
    
    /* Create a new seek event */
    if (rate > 0) {
        seek_event = gst_event_new_seek(rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                       GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_END, 0);
    } else {
        seek_event = gst_event_new_seek(rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                       GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
    }
    
    /* Send the seek event */
    if (!gst_element_send_event(pipeline, seek_event)) {
        g_printerr("Seek operation failed\n");
        return FALSE;
    }
    
    g_print("Position: %" GST_TIME_FORMAT ", Speed: %.2f\n", GST_TIME_ARGS(position), rate);
    return TRUE;
}
```

Compilation:

```bash
gcc -o seek-player seek-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Running:

```bash
./seek-player video.mp4
```

## 10. Practical Examples and Tips

### 10.1. Testing Pipelines with Command-line Tools

GStreamer provides a powerful tool called `gst-launch-1.0` for testing pipelines from the command line. This tool allows you to quickly create and test pipelines without writing code.

Basic usage:

```bash
gst-launch-1.0 [options] ELEMENT [properties] ! ELEMENT [properties] ! ...
```

Examples:

1. **Simple video playback**:
   ```bash
   gst-launch-1.0 playbin uri=file:///path/to/video.mp4
   ```

2. **Test video display**:
   ```bash
   gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
   ```

3. **Audio playback**:
   ```bash
   gst-launch-1.0 audiotestsrc ! audioconvert ! autoaudiosink
   ```

4. **Playing video from file (with elements)**:
   ```bash
   gst-launch-1.0 filesrc location=/path/to/video.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
   ```

5. **Capturing webcam feed**:
   ```bash
   gst-launch-1.0 v4l2src ! videoconvert ! autovideosink
   ```

6. **Converting a video file to a different format**:
   ```bash
   gst-launch-1.0 filesrc location=input.mp4 ! qtdemux ! h264parse ! avdec_h264 ! x264enc ! mp4mux ! filesink location=output.mp4
   ```

7. **Screen recording**:
   ```bash
   gst-launch-1.0 ximagesrc ! videoconvert ! x264enc ! mp4mux ! filesink location=screen-recording.mp4
   ```

These commands help you test your pipeline design before developing your GStreamer application.

### 10.2. Useful Elements

1. **queue**: Creates a queue (buffer) between elements and allows them to run in different threads
   ```c
   GstElement *queue = gst_element_factory_make("queue", "buffer");
   g_object_set(G_OBJECT(queue), "max-size-buffers", 100, NULL);
   ```

2. **tee**: Used to distribute data from a single source to multiple outputs
   ```c
   GstElement *tee = gst_element_factory_make("tee", "stream-divider");
   GstPad *tee_audio_pad = gst_element_get_request_pad(tee, "src_%u");
   GstPad *tee_video_pad = gst_element_get_request_pad(tee, "src_%u");
   ```

3. **fakesink**: Element that consumes data without a real output, for testing data flow
   ```c
   GstElement *fakesink = gst_element_factory_make("fakesink", "test-sink");
   g_object_set(G_OBJECT(fakesink), "sync", TRUE, NULL);
   ```

4. **identity**: Element that passes data through unchanged, optionally providing information about the data
   ```c
   GstElement *identity = gst_element_factory_make("identity", "monitor");
   g_object_set(G_OBJECT(identity), "dump", TRUE, "silent", FALSE, NULL);
   ```

5. **valve**: Control element used to open and close a stream
   ```c
   GstElement *valve = gst_element_factory_make("valve", "control");
   g_object_set(G_OBJECT(valve), "drop", FALSE, NULL);  // TRUE = block data, FALSE = pass data
   ```

### 10.3. Multithreading and GStreamer

GStreamer has an inherently multithreaded architecture. However, you can use certain patterns to manage the interaction between the GStreamer pipeline and the UI thread in your application:

1. **Bus Monitoring with GMainLoop**:
   ```c
   /* Creating a bus watcher */
   GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
   guint bus_watch_id = gst_bus_add_watch(bus, bus_callback, user_data);
   gst_object_unref(bus);### 7.6. Example: Playing a Video File

The following example creates a more complex pipeline to play a video file:

```c
#include <gst/gst.h>

/* Signal handler function prototype */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *demuxer, *video_decoder, *video_convert, *video_sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    
    /* Make sure a media file is specified */
    if (argc != 2) {
        g_printerr("Specify a video file: %s <file>\n", argv[0]);
        return -1;
    }
    
    /* Create elements */
    pipeline = gst_pipeline_new("video-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demuxer = gst_element_factory_make("qtdemux", "demuxer");      // MP4/MOV demuxer
    video_decoder = gst_element_factory_make("avdec_h264", "video-decoder");  // H.264 decoder
    video_convert = gst_element_factory_make("videoconvert", "converter");
    video_sink = gst_element_factory_make("autovideosink", "video-sink");
    
    /* Make sure all elements were created */
    if (!pipeline || !source || !demuxer || !video_decoder || !video_convert || !video_sink) {
        g_printerr("One of the elements could not be created.\n");
        return -1;
    }
    
    /* Set the file location */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Add elements to the pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, video_decoder, video_convert, video_sink, NULL);
    
    /* Start linking elements */
    /* source -> demuxer */
    if (!gst_element_link(source, demuxer)) {
        g_printerr("Elements could not be linked: source -> demuxer\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Add signal watcher for demuxer's h264 video stream */
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), video_decoder);
    
    /* Link video stream to subsequent elements */
    if (!gst_element_link_many(video_decoder, video_convert, video_sink, NULL)) {
        g_printerr("Elements could not be linked: decoder -> converter -> sink\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Set pipeline state to PLAYING */
    g_print("Starting pipeline...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline could not be set to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Wait for messages on the bus */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Message processing */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error %s\n", err->message);
                if (debug_info) {
                    g_printerr("Debug info: %s\n", debug_info);
                }
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                break;
            default:
                g_printerr("Unexpected message received\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Stop the pipeline */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Clean up resources */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}

/* Function called when the demuxer creates a pad */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *decoder = (GstElement *) data;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    const gchar *name;
    
    /* Get the pad's capabilities */
    caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    structure = gst_caps_get_structure(caps, 0);
    name = gst_structure_get_name(structure);
    
    /* We are only interested in video streams */
    if (g_str_has_prefix(name, "video/x-h264")) {
        /* Get the decoder's sink pad */
        sink_pad = gst_element_get_static_pad(decoder, "sink");
        
        /* Link the pads */
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sink_pad))) {
            g_printerr("Pad link from demuxer to video decoder failed.\n");
        } else {
            g_print("Demuxer -> video decoder pad link successful.\n");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
}
```

Compilation:

```bash
gcc -o video-player video-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Running:

```bash
./video-player video.mp4
```



# Examples!

01. This folder contains simple applications in C.
02. This folder contains a media player developed in C++.
03. TODO: To be added
04. TODO: To be added
05. TODO: To be added
06. TODO: To be added
07. TODO: To be added
08. TODO: To be added
09. TODO: To be added
10. TODO: To be added


---

This comprehensive GStreamer tutorial aims to explain GStreamer architecture and usage step by step, from beginner to intermediate level, for C and C++ programmers. The examples are designed to be compilable and executable. Developers who wish to advance to more advanced topics are encouraged to refer to the official GStreamer documentation and examples.

## License

This tutorial is provided under the [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) license. It may be shared, modified, and used for non-commercial purposes with attribution.
