#pragma once
#include "ossrf/gstreamer/api/sender/sender_plugin.h"
#include "ossrf/gstreamer/api/sender/sender_configuration.h"

namespace ossrf::gst::plugins
{
    bisect::expected<gst_sender_plugin_uptr> create_gst_st2110_20_plugin(ossrf::gst::sender::sender_settings settings,
                                                                         ossrf::gst::sender::video_info_t format,
                                                                         int pattern) noexcept;
}