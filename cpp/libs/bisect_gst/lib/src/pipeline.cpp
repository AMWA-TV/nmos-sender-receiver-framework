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

#include "bisect/pipeline.h"
#include "bisect/expected/match.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"

using namespace bisect;
using namespace bisect::gst;

struct pipeline::impl
{
    impl(GstElement* pipeline) : pipeline_(pipeline) {}

    ~impl()
    {
        g_main_loop_quit(gst_loop_);
        this->wait();
        gst_object_unref(GST_OBJECT(pipeline_));
        g_main_loop_unref(gst_loop_);
    }

    void start()
    {
        gst_loop_ = g_main_loop_new(nullptr, FALSE);
        thread_   = std::thread([this]() {
            auto result = this->run();
            if(!result.has_value()) this->handle_error(result.error().what());
        });
    }

    maybe_ok run()
    {
        is_owned_ = false;
        auto bus  = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
        gst_bus_add_watch(bus, &impl::bus_callback, this);
        gst_object_unref(bus);

        /* run */
        BST_ENFORCE(gst_element_set_state(pipeline_, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
                    "Failed changing GStreamer pipeline to Playing");
        g_main_loop_run(gst_loop_);

        /* cleanup */
        BST_ENFORCE(gst_element_set_state(pipeline_, GST_STATE_NULL) != GST_STATE_CHANGE_FAILURE,
                    "Failed changing GStreamer pipeline to Null");

        return {};
    }

    void wait()
    {
        if(thread_.joinable())
        {
            thread_.join();
        }
    }

    void handle_error(const char* message) { g_main_loop_quit(gst_loop_); }

    static gboolean bus_callback(GstBus* /*bus*/, GstMessage* message, gpointer data)
    {
        auto self = static_cast<impl*>(data);

        switch(GST_MESSAGE_TYPE(message))
        {
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug;

            gst_message_parse_error(message, &err, &debug);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(message->src), err->message);
            g_printerr("Debugging information: %s\n", debug ? debug : "none");
            self->handle_error(err->message);
            g_error_free(err);
            g_free(debug);

            break;
        }
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            break;
        case GST_MESSAGE_STATE_CHANGED:
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
            g_print("State changed from %s to %s\n", gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
            break;
        default:
            /* unhandled message */
            break;
        }

        /* we want to be notified again the next time there is a message
         * on the bus, so returning TRUE (FALSE means we want to stop watching
         * for messages on the bus and our callback should not be called again)
         */
        return TRUE;
    }

    GMainLoop* gst_loop_;
    GstElement* pipeline_;
    std::thread thread_;
    bool is_owned_ = true;
};

pipeline::pipeline(GstElement* bin) noexcept : impl_(std::make_unique<impl>(bin))
{
}

pipeline::pipeline() noexcept
{
}

bisect::expected<pipeline> pipeline::create(const char* klass) noexcept
{
    auto bin = gst_pipeline_new(klass);
    BST_ENFORCE(bin != nullptr, "Failed to create GStreamer pipeline");

    return pipeline(bin);
}

pipeline::~pipeline()                           = default;
pipeline::pipeline(pipeline&& other)            = default;
pipeline& pipeline::operator=(pipeline&& other) = default;

GstElement* pipeline::get() noexcept
{
    return impl_->pipeline_;
}

void pipeline::release() noexcept
{
    impl_->is_owned_ = false;
}

void pipeline::run_loop()
{
    impl_->start();
}

maybe_ok pipeline::play()
{
    BST_ENFORCE(gst_element_set_state(impl_->pipeline_, GST_STATE_PLAYING),
                "Failed changing GStreamer pipeline to Playing");
    return {};
}

maybe_ok pipeline::pause()
{
    BST_ENFORCE(gst_element_set_state(impl_->pipeline_, GST_STATE_PAUSED),
                "Failed changing GStreamer pipeline to Pause");
    return {};
}

void pipeline::stop()
{
    g_main_loop_quit(impl_->gst_loop_);
}
