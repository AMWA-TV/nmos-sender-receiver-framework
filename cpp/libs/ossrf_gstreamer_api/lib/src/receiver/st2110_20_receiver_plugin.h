#pragma once
#include "ossrf/gstreamer/api/receiver/receiver_plugin.h"
#include "ossrf/gstreamer/api/receiver/receiver_configuration.h"

namespace ossrf::gst::plugins
{
    bisect::expected<gst_receiver_plugin_uptr>
    create_gst_st2110_20_plugin(ossrf::gst::receiver::receiver_settings settings,
                                ossrf::gst::receiver::video_info_t format) noexcept;
}