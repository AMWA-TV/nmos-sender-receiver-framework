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

#include "st2110_20_receiver_plugin.h"
#include "bisect/expected/macros.h"
#include "bisect/pipeline.h"
#include <gst/gst.h>

using namespace bisect;
using namespace ossrf::gst::receiver;
using namespace ossrf::gst::plugins;

namespace
{
    constexpr auto queue_max_size_time    = 200000;
    constexpr auto queue_max_size_buffers = 0;
    constexpr auto queue_max_size_bytes   = 0;
}; // namespace

struct gst_st2110_20_receiver_impl : gst_receiver_plugin_t
{
    receiver_settings s_;
    video_info_t f_;
    gst::pipeline pipeline_;

    gst_st2110_20_receiver_impl(receiver_settings settings, video_info_t format) : s_(settings), f_(format) {}

    ~gst_st2110_20_receiver_impl() { stop(); }

    maybe_ok create_gstreamer_pipeline()
    {
        // Create pipeline and check if all elements are created successfully
        BST_CHECK_ASSIGN(pipeline_, bisect::gst::pipeline::create(NULL));
        auto* pipeline = pipeline_.get();

        // Add pipeline udp source
        auto* source = gst_element_factory_make("udpsrc", NULL);
        BST_ENFORCE(source != nullptr, "Failed creating GStreamer element udpsrc");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), source), "Failed adding udpsrc to the pipeline");

        // Set udp source params
        g_object_set(G_OBJECT(source), "address", s_.primary.source_ip_address.c_str(), NULL);
        g_object_set(G_OBJECT(source), "auto-multicast", TRUE, NULL);
        g_object_set(G_OBJECT(source), "port", s_.primary.source_port, NULL);
        g_object_set(G_OBJECT(source), "multicast-iface", s_.primary.interface_name.c_str(), NULL);

        // Create and set caps for udp source
        GstCaps* caps = gst_caps_from_string(
            "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)RAW, "
            "sampling=(string)RGB, width=(string)640, height=(string)480");
        g_object_set(G_OBJECT(source), "caps", caps, NULL);

        // Add pipeline rtpjitterbuffer
        auto* jitter_buffer = gst_element_factory_make("rtpjitterbuffer", NULL);
        BST_ENFORCE(jitter_buffer != nullptr, "Failed creating GStreamer element jitter_buffer");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), jitter_buffer), "Failed adding jitter_buffer to the pipeline");

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");
        g_object_set(G_OBJECT(queue1), "max-size-time", queue_max_size_time, "max-size-buffers", queue_max_size_buffers,
                     "max-size-bytes", queue_max_size_bytes, NULL);

        // Add pipeline rtp depay
        auto* depay = gst_element_factory_make("rtpvrawdepay", NULL);
        BST_ENFORCE(depay != nullptr, "Failed creating GStreamer element depay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), depay), "Failed adding depay to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");
        g_object_set(G_OBJECT(queue1), "max-size-time", queue_max_size_time, "max-size-buffers", queue_max_size_buffers,
                     "max-size-bytes", queue_max_size_bytes, NULL);

        // Add pipeline videoconvert
        auto* videoconvert = gst_element_factory_make("videoconvert", NULL);
        BST_ENFORCE(videoconvert != nullptr, "Failed creating GStreamer element converter");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), videoconvert), "Failed adding converter to the pipeline");

        // Add pipeline video sink
        auto* sink = gst_element_factory_make("autovideosink", NULL);
        BST_ENFORCE(sink != nullptr, "Failed creating GStreamer element sink");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), sink), "Failed adding sink to the pipeline");

        // Link all elements together
        BST_ENFORCE(gst_element_link_many(source, queue1, depay, queue2, videoconvert, sink, NULL),
                    "Failed linking GStreamer video pipeline");

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

// TODO this function will need to receive an SDP and use the information in it to build the GST pipeline
expected<gst_receiver_plugin_uptr> ossrf::gst::plugins::create_gst_st2110_20_plugin(receiver_settings settings,
                                                                                    video_info_t format) noexcept
{
    auto i = std::make_unique<gst_st2110_20_receiver_impl>(settings, format);

    BST_CHECK(i->create_gstreamer_pipeline());

    return i;
}
