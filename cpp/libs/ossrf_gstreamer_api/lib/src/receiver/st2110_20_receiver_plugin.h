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

#pragma once
#include "ossrf/gstreamer/api/receiver/receiver_plugin.h"
#include "ossrf/gstreamer/api/receiver/receiver_configuration.h"

namespace ossrf::gst::plugins
{
    bisect::expected<gst_receiver_plugin_uptr>
    create_gst_st2110_20_plugin(ossrf::gst::receiver::receiver_settings settings,
                                ossrf::gst::receiver::video_info_t format) noexcept;
}