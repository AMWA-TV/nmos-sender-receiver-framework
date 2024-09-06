#include "bisect/expected/match.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"
#include "bisect/pipeline.h"
#include "bisect/initializer.h"
#include <gst/gst.h>
#include <fmt/core.h>
#include <iostream>
#include <fstream>

namespace
{
    static void decoder_pad_added(GstElement* src, GstPad* new_pad, gpointer data)
    {
        (void)src;
        auto* sink     = GST_ELEMENT(data);
        auto* sink_pad = gst_element_get_static_pad(sink, "sink");
        if(!gst_pad_is_linked(sink_pad))
        {
            if(gst_pad_link(new_pad, sink_pad) == GST_PAD_LINK_OK)
            {
                g_print("Decoder linked to convert\n");
            }
            else
            {
                g_printerr("Failed to link decoder to convert\n");
            }
        }
        gst_object_unref(sink_pad);
    }

    bool file_exists(const std::string& filename)
    {
        std::ifstream file(filename);
        return file.good();
    }

    bisect::maybe_ok gstreamer_pipeline_video()
    {
        // Create pipeline and check if all elements are created successfully
        BST_ASSIGN_MUT(pipeline_holder, bisect::gst::pipeline::create("sender_pipeline"));
        auto* pipeline = pipeline_holder.get();

        // Add pipeline videotestsrc
        auto* source = gst_element_factory_make("videotestsrc", "source");
        BST_ENFORCE(source != nullptr, "Failed creating GStreamer element videotestsrc");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), source), "Failed adding videotestsrc to the pipeline");

        // Add pipeline capsfilter
        auto* capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
        BST_ENFORCE(capsfilter != nullptr, "Failed creating capsfilter");
        // Create caps for capsfilter
        auto* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, 640,
                                         "height", G_TYPE_INT, 480, NULL);
        BST_ENFORCE(caps != nullptr, "Failed creating GStreamer video caps");
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), capsfilter), "Failed adding capsfilter to the pipeline");
        gst_caps_unref(caps);

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", "queue1");
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");

        // Add pipeline rtpvrawpay
        auto* rtpvrawpay = gst_element_factory_make("rtpvrawpay", "rtpvrawpay");
        BST_ENFORCE(rtpvrawpay != nullptr, "Failed creating GStreamer element rtpvrawpay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), rtpvrawpay), "Failed adding rtpvrawpay to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", "queue2");
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");

        // Add pipeline udpsink
        auto* udpsink = gst_element_factory_make("udpsink", "udpsink");
        BST_ENFORCE(udpsink != nullptr, "Failed creating GStreamer element udpsink");
        // Set properties
        g_object_set(G_OBJECT(udpsink), "host", "239.100.2.1", NULL);
        g_object_set(G_OBJECT(udpsink), "port", 6000, NULL);
        g_object_set(G_OBJECT(udpsink), "auto-multicast", TRUE, NULL);
        g_object_set(G_OBJECT(udpsink), "multicast-iface", "eno2", NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), udpsink), "Failed adding udpsink to the pipeline");

        // Link elements
        BST_ENFORCE(gst_element_link_many(source, capsfilter, queue1, rtpvrawpay, queue2, udpsink, NULL),
                    "Failed linking GStreamer video pipeline");

        // Setup runner
        pipeline_holder.run_loop();

        return {};
    }

    bisect::maybe_ok gstreamer_pipeline_video(const std::string& file_video)
    {
        // Create pipeline and check if all elements are created successfully
        BST_ASSIGN_MUT(pipeline_holder, bisect::gst::pipeline::create("video-sender"));
        auto* pipeline = pipeline_holder.get();

        // Add pipeline filesrc
        auto* filesrc = gst_element_factory_make("filesrc", "file-source");
        BST_ENFORCE(filesrc != nullptr, "Failed creating GStreamer element filesrc");
        g_object_set(G_OBJECT(filesrc), "location", file_video.c_str(), NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), filesrc), "Failed adding filesrc to the pipeline");

        // Add pipeline decoder
        auto* decoder = gst_element_factory_make("decodebin", "decoder");
        BST_ENFORCE(decoder != nullptr, "Failed creating GStreamer element decodebin");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), decoder), "Failed adding decodebin to the pipeline");

        // Add pipeline videoconvert
        auto* videoconvert = gst_element_factory_make("videoconvert", "video-convert");
        BST_ENFORCE(videoconvert != nullptr, "Failed creating GStreamer element videoconvert");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), videoconvert), "Failed adding videoconvert to the pipeline");

        // Add pipeline videoscale
        auto* videoscale = gst_element_factory_make("videoscale", "video-scale");
        BST_ENFORCE(videoscale != nullptr, "Failed creating GStreamer element videoscale");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), videoscale), "Failed adding videoscale to the pipeline");

        // Add pipeline capsfilter
        auto* capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
        BST_ENFORCE(capsfilter != nullptr, "Failed creating GStreamer element capsfilter");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), capsfilter), "Failed adding capsfilter to the pipeline");

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", "queue1");
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");

        // Add pipeline rtpvrawpay
        auto* rtpvrawpay = gst_element_factory_make("rtpvrawpay", "rtpvrawpay");
        BST_ENFORCE(rtpvrawpay != nullptr, "Failed creating GStreamer element rtpvrawpay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), rtpvrawpay), "Failed adding rtpvrawpay to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", "queue2");
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");

        // Add pipeline udpsink
        auto* udpsink = gst_element_factory_make("udpsink", "udpsink");
        BST_ENFORCE(udpsink != nullptr, "Failed creating GStreamer element udpsink");
        // Set properties
        g_object_set(G_OBJECT(udpsink), "host", "239.100.2.1", "port", 6000, "auto-multicast", TRUE, "multicast-iface",
                     "eno2", NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), udpsink), "Failed adding udpsink to the pipeline");

        // Link elements
        BST_ENFORCE(gst_element_link_many(filesrc, decoder, NULL),
                    "Failed linking GStreamer elements filesrc and decodebin");

        // Set caps for capsfilter
        auto* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, 640,
                                         "height", G_TYPE_INT, 480, NULL);
        BST_ENFORCE(caps != nullptr, "Failed creating GStreamer video caps");
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        gst_caps_unref(caps);

        // Connect decoder's pad-added signal
        BST_ENFORCE(g_signal_connect(decoder, "pad-added", G_CALLBACK(decoder_pad_added), videoconvert) > 0,
                    "Failed signaling pad-added in decodebin");

        BST_ENFORCE(
            gst_element_link_many(videoconvert, videoscale, capsfilter, queue1, rtpvrawpay, queue2, udpsink, NULL),
            "Failed linking elements in GStreamer video pipeline");

        // Setup runner
        pipeline_holder.run_loop();

        return {};
    }

    bisect::maybe_ok gstreamer_pipeline_audio()
    {
        // Create pipeline and check if all elements are created successfully
        BST_ASSIGN_MUT(pipeline_holder, bisect::gst::pipeline::create("audio-sender"));
        auto* pipeline = pipeline_holder.get();

        // Add pipeline pulsesrc
        auto* source = gst_element_factory_make("pulsesrc", "source");
        BST_ENFORCE(source != nullptr, "Failed creating GStreamer element pulsesrc");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), source), "Failed adding pulsesrc to the pipeline");

        // Add pipeline audioconvert
        auto* convert = gst_element_factory_make("audioconvert", "convert");
        BST_ENFORCE(convert != nullptr, "Failed creating GStreamer element audioconvert");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), convert), "Failed adding audioconvert to the pipeline");

        // Add pipeline videotestsrc
        auto* resample = gst_element_factory_make("audioresample", "resample");
        BST_ENFORCE(resample != nullptr, "Failed creating GStreamer element audioresample");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), resample), "Failed adding audioresample to the pipeline");

        // Add pipeline capsfilter
        auto* capsfilter = gst_element_factory_make("capsfilter", "caps");
        BST_ENFORCE(capsfilter != nullptr, "Failed creating GStreamer element capsfilter");
        // Create caps for capsfilter
        auto* caps = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, 2, "rate", G_TYPE_INT, 48000, NULL);
        BST_ENFORCE(caps != nullptr, "Failed creating GStreamer audio caps");
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), capsfilter), "Failed adding capsfilter to the pipeline");
        gst_caps_unref(caps);

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", "queue1");
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");

        // Add pipeline payloader
        auto* payloader = gst_element_factory_make("rtpL24pay", "payloader");
        BST_ENFORCE(payloader != nullptr, "Failed creating GStreamer element rtpL24pay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), payloader), "Failed adding rtpL24pay to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", "queue2");
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");

        // Add pipeline udpsink
        auto* sink = gst_element_factory_make("udpsink", "sink");
        BST_ENFORCE(sink != nullptr, "Failed creating GStreamer element udpsink");
        // Set properties for udpsink
        g_object_set(G_OBJECT(sink), "host", "239.100.2.1", "port", 6000, "auto-multicast", TRUE, "multicast-iface",
                     "eno2", NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), sink), "Failed adding udpsink to the pipeline");

        // Link elements
        BST_ENFORCE(gst_element_link_many(source, convert, resample, capsfilter, queue1, payloader, queue2, sink, NULL),
                    "Failed linking GStreamer elements in audio pipeline");

        // Setup runner
        pipeline_holder.run_loop();

        return {};
    }

    bisect::maybe_ok gstreamer_pipeline_audio(const std::string& file_music)
    {
        // Create pipeline and check if all elements are created successfully
        BST_ASSIGN_MUT(pipeline_holder, bisect::gst::pipeline::create("audio-sender"));
        auto* pipeline = pipeline_holder.get();

        // Add pipeline filesrc
        auto* filesrc = gst_element_factory_make("filesrc", "file-source");
        BST_ENFORCE(filesrc != nullptr, "Failed creating GStreamer element filesrc");
        g_object_set(G_OBJECT(filesrc), "location", file_music.c_str(), NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), filesrc), "Failed adding filesrc to the pipeline");

        // Add pipeline decodebin
        auto* decoder = gst_element_factory_make("decodebin", "decoder");
        BST_ENFORCE(decoder != nullptr, "Failed creating GStreamer element decodebin");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), decoder), "Failed adding decodebin to the pipeline");

        // Add pipeline audioconvert
        auto* audioconvert = gst_element_factory_make("audioconvert", "audio-convert");
        BST_ENFORCE(audioconvert != nullptr, "Failed creating GStreamer element audioconvert");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), audioconvert), "Failed adding audioconvert to the pipeline");

        // Add pipeline audioresample
        auto* audioresample = gst_element_factory_make("audioresample", "audio-resample");
        BST_ENFORCE(audioresample != nullptr, "Failed creating GStreamer element audioresample");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), audioresample), "Failed adding audioresample to the pipeline");

        // Add pipeline capsfilter
        auto* capsfilter = gst_element_factory_make("capsfilter", "caps-filter");
        BST_ENFORCE(capsfilter != nullptr, "Failed creating GStreamer element capsfilter");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), capsfilter), "Failed adding capsfilter to the pipeline");

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", "queue1");
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");

        // Add pipeline payloader
        auto* payloader = gst_element_factory_make("rtpL24pay", "payloader");
        BST_ENFORCE(payloader != nullptr, "Failed creating GStreamer element rtpL24pay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), payloader), "Failed adding rtpL24pay to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", "queue2");
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");

        // Add pipeline udpsink
        auto* udpsink = gst_element_factory_make("udpsink", "udp-sink");
        BST_ENFORCE(udpsink != nullptr, "Failed creating GStreamer element udpsink");
        // Set properties
        g_object_set(G_OBJECT(udpsink), "host", "239.100.2.1", "port", 6000, "auto-multicast", TRUE, "multicast-iface",
                     "eno2", NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), udpsink), "Failed adding udpsink to the pipeline");

        // Link elements
        BST_ENFORCE(gst_element_link_many(filesrc, decoder, NULL),
                    "Failed linking GStreamer elements filsrc and decodebin");

        // Set caps for capsfilter
        auto* caps = gst_caps_new_simple("audio/x-raw", "media", G_TYPE_STRING, "audio", "payload", G_TYPE_INT, 96,
                                         "clock-rate", G_TYPE_INT, 48000, "channels", G_TYPE_INT, 2, NULL);
        BST_ENFORCE(caps != nullptr, "Failed creating GStreamer audio caps");
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        gst_caps_unref(caps);

        // Connect decoder's pad-added signal
        BST_ENFORCE(g_signal_connect(decoder, "pad-added", G_CALLBACK(decoder_pad_added), audioconvert) > 0,
                    "Failed signaling pad-added in decodebin");

        // Link remaining elements
        BST_ENFORCE(gst_element_link_many(audioconvert, audioresample, capsfilter, payloader, udpsink, NULL),
                    "Failed linking GStreamer elements in audio pipeline");

        // Setup runner
        pipeline_holder.run_loop();

        return {};
    }
} // namespace

int main(int argc, char* argv[])
{
    bisect::gst::initializer initializer;

    switch(argc)
    {
    case 2:
        if(argv[1] == std::string("-a"))
        {
            auto result = gstreamer_pipeline_audio();
            if(!result.has_value())
            {
                fprintf(stderr, "error: %s", result.error().what());
                return -1;
            }
            return 0;
        }
        else if(argv[1] == std::string("-v"))
        {
            auto result = gstreamer_pipeline_video();
            if(!result.has_value())
            {
                fprintf(stderr, "error: %s", result.error().what());
                return -1;
            }
            return 0;
        }
        fprintf(stderr, "error: %s option doesn't exist \n\n", argv[1]);
        break;
    case 3:

        if(!file_exists(argv[2]))
        {
            fprintf(stderr, "error: file %s doesn't exist \n", argv[2]);
            return -1;
        }

        if(argv[1] == std::string("-a"))
        {
            auto result = gstreamer_pipeline_audio(argv[2]);
            if(!result.has_value())
            {
                fprintf(stderr, "error: %s", result.error().what());
                return -1;
            }
            return 0;
        }
        else if(argv[1] == std::string("-v"))
        {
            auto result = gstreamer_pipeline_video(argv[2]);
            if(!result.has_value())
            {
                fprintf(stderr, "error: %s", result.error().what());
                return -1;
            }
            return 0;
        }
        fprintf(stderr, "error: %s option doesn't exist \n\n", argv[1]);
        break;

    default: break;
    }

    fprintf(stderr, "usage: %s -a               -----> simple audio pipeline\n", argv[0]);
    fprintf(stderr, "       %s -a [file_source] -----> file sourced audio pipeline\n", argv[0]);
    fprintf(stderr, "       %s -v               -----> simple video pipeline\n", argv[0]);
    fprintf(stderr, "       %s -v [file_source] -----> file sourced video pipeline\n\n", argv[0]);
    return -1;
}
