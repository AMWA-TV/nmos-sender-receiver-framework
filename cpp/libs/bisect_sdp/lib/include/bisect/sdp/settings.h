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

#include "bisect/nmoscpp/configuration.h"
#include "bisect/sdp/clocks.h"
#include <string>
#include <variant>

namespace bisect::sdp
{
    struct rtp_settings_t
    {
        uint8_t payload_type; // 96..127
    };

    struct origin_settings_t
    {
        std::string description;
        std::string session_id;
        std::string session_version;
    };

    struct sdp_settings_t
    {
        using format_t = std::variant<bisect::nmoscpp::video_sender_info_t, bisect::nmoscpp::audio_sender_info_t>;

        format_t format;
        origin_settings_t origin;
        rtp_settings_t rtp;
        bisect::nmoscpp::network_leg_t primary;
        std::optional<bisect::nmoscpp::network_leg_t> secondary;
        refclk_t ts_refclk;
        mediaclk_t mediaclk;
    };
} // namespace bisect::sdp
