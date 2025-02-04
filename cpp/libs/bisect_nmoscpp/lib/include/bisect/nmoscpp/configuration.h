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

#include "bisect/nmoscpp/detail/internal.h"
#include <vector>
#include <variant>
#include <optional>
#include <nmos/rational.h>
#include <nmos/format.h>
#include <nmos/media_type.h>
#include <nmos/interlace_mode.h>
#include <cpprest/json.h>
#include <nlohmann/json.hpp>

namespace bisect::nmoscpp
{
    struct network_leg_t
    {
        bool rtp_enabled;
        std::optional<int> source_port;
        std::optional<std::string> source_ip;
        std::optional<std::string> destination_ip;
        std::optional<int> destination_port;
        std::optional<std::string> interface_name;
        std::optional<std::string> interface_ip;
    };

    struct network_t
    {
        network_leg_t primary;
        std::optional<network_leg_t> secondary;
    };

    namespace capabilities
    {
        struct video_h265_t
        {
            int width;
            int height;
            nmos::rational exact_framerate;
            std::string color_sampling;
        };

        struct video_h264_t
        {
            int width;
            int height;
            nmos::rational exact_framerate;
            std::string color_sampling;
        };

        struct video_st2110_20_t
        {
            int width;
            int height;
            nmos::rational exact_framerate;
            std::string color_sampling;
        };

        struct audio_st2110_30_t
        {
            int channels;
            int bits_per_sample;
            int samplerate;
        };
    } // namespace capabilities

    using capabilities_t = std::variant<capabilities::video_h264_t, capabilities::video_h265_t,
                                        capabilities::video_st2110_20_t, capabilities::audio_st2110_30_t>;

    struct meta_info_t
    {
        std::string id;
        std::string label;
        std::string description;
    };

    struct nmos_receiver_t : meta_info_t
    {
        bool master_enable = false;
        std::optional<std::string> sender_id;
        std::optional<std::string> sdp_data;
        network_t network;
        std::string protocol;
        nmos::format format;
        std::vector<nmos::media_type> media_types;
        capabilities_t capabilities;
    };

    struct video_sender_info_t
    {
        int height = 0;
        int width  = 0;
        nmos::rational exact_framerate;
        std::string chroma_sub_sampling;
        nmos::interlace_mode structure;
        int depth = 0;
    };

    struct audio_sender_info_t
    {
        int number_of_channels       = 0;
        unsigned int bits_per_sample = 0;
        int sampling_rate            = 0;
        float packet_time            = 0.;
    };

    using sender_media_info_t = std::variant<video_sender_info_t, audio_sender_info_t>;

    struct source_t : meta_info_t
    {
    };

    struct flow_t : meta_info_t
    {
        // Additional flow properties
        web::json::value extra;
    };

    struct nmos_sender_t : meta_info_t
    {
        bool master_enable;
        network_t network;
        std::string protocol;
        nmos::format format;
        nmos::rational grain_rate;
        std::string media_type;
        source_t source;
        flow_t flow;
        sender_media_info_t media;
        std::optional<uint8_t> payload_type;
        std::optional<std::string> forced_sdp;

        // Additional sender properties
        web::json::value extra;
    };

    struct nmos_device_t : meta_info_t
    {
        std::string node_id;
    };

    using sender_activation_callback_t =
        std::function<void(bool master_enable, const nlohmann::json& transport_params)>;

    using receiver_activation_callback_t =
        std::function<void(const std::optional<std::string>& sdp, const bool master_enable, const nlohmann::json& transport_params)>;
} // namespace bisect::nmoscpp
