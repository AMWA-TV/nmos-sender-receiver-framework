// Copyright (C) 2024 Advanced Media Workflow Association
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "st2110_30_sender_plugin.h"
#include "bisect/expected/macros.h"
#include "bisect/pipeline.h"
#include <gst/gst.h>

using namespace bisect;
using namespace ossrf::gst::sender;
using namespace ossrf::gst::plugins;

namespace
{
    constexpr auto queue_max_size_time    = 200000;
    constexpr auto queue_max_size_buffers = 0;
    constexpr auto queue_max_size_bytes   = 0;
}; // namespace

struct gst_st2110_30_sender_impl : gst_sender_plugin_t
{
    sender_settings s_;
    audio_info_t f_;
    gst::pipeline pipeline_;

    gst_st2110_30_sender_impl(sender_settings settings, audio_info_t format) : s_(settings), f_(format) {}

    ~gst_st2110_30_sender_impl() { stop(); }

    maybe_ok create_gstreamer_pipeline()
    {
        // Create pipeline and check if all elements are created successfully
        BST_CHECK_ASSIGN(pipeline_, bisect::gst::pipeline::create(NULL));
        auto* pipeline = pipeline_.get();

        // Add pipeline audiotestsrc (audio source)
        auto* source = gst_element_factory_make("audiotestsrc", NULL);
        BST_ENFORCE(source != nullptr, "Failed creating GStreamer element audiotestsrc");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), source), "Failed adding audiotestsrc to the pipeline");

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        g_object_set(G_OBJECT(queue1), "max-size-time", queue_max_size_time, "max-size-buffers", queue_max_size_buffers,
                     "max-size-bytes", queue_max_size_bytes, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");

        // Add pipeline audioconvert
        auto* audioconvert = gst_element_factory_make("audioconvert", NULL);
        BST_ENFORCE(audioconvert != nullptr, "Failed creating GStreamer element audioconvert");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), audioconvert), "Failed adding audioconvert to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        g_object_set(G_OBJECT(queue1), "max-size-time", queue_max_size_time, "max-size-buffers", queue_max_size_buffers,
                     "max-size-bytes", queue_max_size_bytes, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");

        // Add pipeline audioresample
        auto* audioresample = gst_element_factory_make("audioresample", NULL);
        BST_ENFORCE(audioresample != nullptr, "Failed creating GStreamer element audioresample");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), audioresample), "Failed adding audioresample to the pipeline");

        // Add pipeline capsfilter
        auto* capsfilter = gst_element_factory_make("capsfilter", NULL);
        BST_ENFORCE(capsfilter != nullptr, "Failed creating capsfilter");

        // Create caps for capsfilter
        auto* caps = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, f_.number_of_channels, "rate",
                                         G_TYPE_INT, f_.sampling_rate, NULL);
        BST_ENFORCE(caps != nullptr, "Failed creating GStreamer audio caps");
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), capsfilter), "Failed adding capsfilter to the pipeline");
        gst_caps_unref(caps);

        // Add pipeline queue1
        auto* queue3 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue3 != nullptr, "Failed creating GStreamer element queue");
        g_object_set(G_OBJECT(queue1), "max-size-time", queue_max_size_time, "max-size-buffers", queue_max_size_buffers,
                     "max-size-bytes", queue_max_size_bytes, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue3), "Failed adding queue to the pipeline");

        // Add pipeline rtpL24pay (audio payload)
        auto* rtpL24pay = gst_element_factory_make("rtpL24pay", NULL);
        BST_ENFORCE(rtpL24pay != nullptr, "Failed creating GStreamer element rtpL24pay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), rtpL24pay), "Failed adding rtpL24pay to the pipeline");

        // Add pipeline udpsink
        auto* udpsink = gst_element_factory_make("udpsink", NULL);
        BST_ENFORCE(udpsink != nullptr, "Failed creating GStreamer element udpsink");
        // Set properties
        g_object_set(G_OBJECT(udpsink), "host", s_.primary.destination_ip_address.c_str(), NULL);
        g_object_set(G_OBJECT(udpsink), "port", s_.primary.destination_port, NULL);
        g_object_set(G_OBJECT(udpsink), "auto-multicast", TRUE, NULL);
        g_object_set(G_OBJECT(udpsink), "multicast-iface", s_.primary.interface_name.c_str(), NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), udpsink), "Failed adding udpsink to the pipeline");

        // Link elements
        BST_ENFORCE(gst_element_link_many(source, queue1, audioconvert, queue2, audioresample, capsfilter, queue3,
                                          rtpL24pay, udpsink, NULL),
                    "Failed linking GStreamer audio pipeline");

        // Setup runner
        pipeline_.run_loop();

        return {};
    }

    void stop() noexcept override
    {
        pipeline_.stop();
        pipeline_ = {};
    }
};

expected<gst_sender_plugin_uptr> ossrf::gst::plugins::create_gst_st2110_30_plugin(sender_settings settings,
                                                                                  audio_info_t format) noexcept
{
    auto i = std::make_unique<gst_st2110_30_sender_impl>(settings, format);

    BST_CHECK(i->create_gstreamer_pipeline());

    return i;
}
