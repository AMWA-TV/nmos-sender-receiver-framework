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
#include <string>
#include <optional>
#include <variant>

namespace ossrf::gst::receiver
{
    struct network_settings_t
    {
        std::string interface_name;
        std::string interface_address;
        std::string source_ip_address;
        uint16_t source_port;
    };

    struct video_info_t
    {
        int height;
        int width;
        std::string chroma_sub_sampling;
    };

    struct audio_info_t
    {
        int number_of_channels;
        unsigned int bits_per_sample;
        int sampling_rate;
        float packet_time;
    };

    using format_t = std::variant<video_info_t, audio_info_t>;

    struct receiver_settings
    {
        format_t format;
        network_settings_t primary;
        std::optional<network_settings_t> secondary;
    };
} // namespace ossrf::gst::receiver