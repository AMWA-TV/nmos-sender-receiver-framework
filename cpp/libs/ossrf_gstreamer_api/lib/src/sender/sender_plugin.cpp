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

#include "ossrf/gstreamer/api/sender/sender_plugin.h"
#include "ossrf/gstreamer/api/sender/sender_configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected/match.h"
#include "bisect/json.h"
#include "st2110_20_sender_plugin.h"
#include "st2110_30_sender_plugin.h"
#include <nlohmann/json.hpp>

using namespace bisect;
using namespace ossrf::gst;
using namespace ossrf::gst::sender;
using namespace ossrf::gst::plugins;
using json = nlohmann::json;

namespace
{
    expected<frame_rate_t> framerate_from_json(const json& config)
    {
        BST_ASSIGN(numerator, find<uint32_t>(config, "num"));
        BST_ASSIGN(denominator, find<uint32_t>(config, "den"));
        return frame_rate_t{.num = numerator, .den = denominator};
    }

    expected<frame_structure_t> to_interlace_mode(const std::string& s)
    {
        if(s == "progressive") return frame_structure_t::progressive;
        if(s == "interlaced_tff") return frame_structure_t::interlaced_tff;
        if(s == "interlaced_bff") return frame_structure_t::interlaced_bff;
        if(s == "interlaced_psf") return frame_structure_t::psf;
        BST_FAIL("invalid frame structure '{}'", s);
    }

    expected<video_info_t> video_sender_info_from_json(const json& media)
    {
        video_info_t info;
        BST_ASSIGN(frame_rate, find<json>(media, "frame_rate"));
        BST_CHECK_ASSIGN(info.exact_framerate, framerate_from_json(frame_rate));
        BST_CHECK_ASSIGN(info.chroma_sub_sampling, find<std::string>(media, "sampling"));
        BST_CHECK_ASSIGN(info.width, find<int>(media, "width"));
        BST_CHECK_ASSIGN(info.height, find<int>(media, "height"));
        BST_ASSIGN(structure_s, find<std::string>(media, "structure"));
        BST_ASSIGN(structure, to_interlace_mode(structure_s));
        info.structure = structure;

        return info;
    }

    expected<audio_info_t> audio_l24_sender_info_from_json(const json& media)
    {
        audio_info_t info;
        BST_CHECK_ASSIGN(info.number_of_channels, find<int>(media, "number_of_channels"));
        BST_CHECK_ASSIGN(info.sampling_rate, find<int>(media, "sampling_rate"));
        BST_CHECK_ASSIGN(info.packet_time, find<float>(media, "packet_time"));
        info.bits_per_sample = 24;
        return info;
    }

    expected<network_settings_t> network_from_json(const json& config)
    {
        network_settings_t net;
        BST_ASSIGN(primary, find<json>(config, "primary"));
        assign_if<std::string>(primary, "destination_address", net, &network_settings_t::destination_ip_address);
        assign_if<uint16_t>(primary, "destination_port", net, &network_settings_t::destination_port);
        assign_if<std::string>(primary, "source_address", net, &network_settings_t::source_ip_address);
        assign_if<std::string>(primary, "interface_name", net, &network_settings_t::interface_name);

        return net;
    }

    expected<sender_settings> translate_json(const json& config)
    {
        sender_settings s;
        BST_ASSIGN(network, find<json>(config, "network"));
        BST_CHECK_ASSIGN(s.primary, network_from_json(network));

        BST_ASSIGN(media_type, find<std::string>(config, "media_type"));

        BST_ASSIGN(media, find<json>(config, "media"));

        if(media_type == "video/raw")
        {
            BST_CHECK_ASSIGN(s.format, video_sender_info_from_json(media));
        }
        else if(media_type == "audio/L24")
        {
            BST_CHECK_ASSIGN(s.format, audio_l24_sender_info_from_json(media));
        }
        else
        {
            BST_FAIL("invalid media type: {}", media_type);
        }

        return s;
    }

    template <typename In>
    expected<gst_sender_plugin_uptr> do_create_plugin(const In&, const sender_settings&, int pattern)
    {
        BST_FAIL("invalid format");
    }

    expected<gst_sender_plugin_uptr> do_create_plugin(const video_info_t& format, const sender_settings& settings,
                                                      int pattern)
    {
        return create_gst_st2110_20_plugin(settings, format, pattern);
    }

    expected<gst_sender_plugin_uptr> do_create_plugin(const audio_info_t& format, const sender_settings& settings,
                                                      int pattern)
    {
        return create_gst_st2110_30_plugin(settings, format);
    }
} // namespace

expected<gst_sender_plugin_uptr> plugins::create_gst_sender_plugin(const std::string& config, int pattern) noexcept
{

    BST_ASSIGN(s, translate_json(json::parse(config)));

    return match(s.format, overload{[&](const auto& f) { return do_create_plugin(f, s, pattern); }});
}