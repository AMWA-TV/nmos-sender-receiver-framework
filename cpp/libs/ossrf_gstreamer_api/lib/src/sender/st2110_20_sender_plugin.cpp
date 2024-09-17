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

#include "st2110_20_sender_plugin.h"
#include "bisect/expected/macros.h"
#include "bisect/pipeline.h"
#include <gst/gst.h>

using namespace bisect;
using namespace ossrf::gst::sender;
using namespace ossrf::gst::plugins;

struct gst_st2110_20_sender_impl : gst_sender_plugin_t
{
    sender_settings s_;
    video_info_t f_;
    gst::pipeline pipeline_;

    gst_st2110_20_sender_impl(sender_settings settings, video_info_t format) : s_(settings), f_(format) {}

    ~gst_st2110_20_sender_impl() { stop(); }

    maybe_ok create_gstreamer_pipeline(int pattern)
    {
        // Create pipeline and check if all elements are created successfully
        BST_CHECK_ASSIGN(pipeline_, bisect::gst::pipeline::create(NULL));
        auto* pipeline = pipeline_.get();

        // Add pipeline videotestsrc
        auto* source = gst_element_factory_make("videotestsrc", NULL);
        BST_ENFORCE(source != nullptr, "Failed creating GStreamer element videotestsrc");
        g_object_set(G_OBJECT(source), "pattern", pattern, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), source), "Failed adding videotestsrc to the pipeline");

        // Add pipeline capsfilter
        auto* capsfilter = gst_element_factory_make("capsfilter", NULL);
        BST_ENFORCE(capsfilter != nullptr, "Failed creating capsfilter");

        // Create caps for capsfilter
        auto* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, f_.width,
                                         "height", G_TYPE_INT, f_.height, NULL);
        BST_ENFORCE(caps != nullptr, "Failed creating GStreamer video caps");
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), capsfilter), "Failed adding capsfilter to the pipeline");
        gst_caps_unref(caps);

        // Add pipeline queue1
        auto* queue1 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue1 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue1), "Failed adding queue to the pipeline");

        // Add pipeline rtpvrawpay
        auto* rtpvrawpay = gst_element_factory_make("rtpvrawpay", NULL);
        BST_ENFORCE(rtpvrawpay != nullptr, "Failed creating GStreamer element rtpvrawpay");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), rtpvrawpay), "Failed adding rtpvrawpay to the pipeline");

        // Add pipeline queue2
        auto* queue2 = gst_element_factory_make("queue", NULL);
        BST_ENFORCE(queue2 != nullptr, "Failed creating GStreamer element queue");
        BST_ENFORCE(gst_bin_add(GST_BIN(pipeline), queue2), "Failed adding queue to the pipeline");

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
        BST_ENFORCE(gst_element_link_many(source, capsfilter, queue1, rtpvrawpay, queue2, udpsink, NULL),
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

expected<gst_sender_plugin_uptr>
ossrf::gst::plugins::create_gst_st2110_20_plugin(sender_settings settings, video_info_t format, int pattern) noexcept
{
    auto i = std::make_unique<gst_st2110_20_sender_impl>(settings, format);

    BST_CHECK(i->create_gstreamer_pipeline(pattern));

    return i;
}