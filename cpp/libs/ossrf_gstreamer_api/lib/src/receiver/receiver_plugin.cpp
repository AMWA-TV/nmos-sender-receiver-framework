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

#include "ossrf/gstreamer/api/receiver/receiver_plugin.h"
#include "ossrf/gstreamer/api/receiver/receiver_configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected/match.h"
#include "bisect/json.h"
#include "bisect/sdp/reader.h"
#include "st2110_20_receiver_plugin.h"
#include <nlohmann/json.hpp>

using namespace bisect;
using namespace bisect::sdp;
using namespace bisect::nmoscpp;
using namespace ossrf::gst;
using namespace ossrf::gst::receiver;
using namespace ossrf::gst::plugins;
using json = nlohmann::json;

namespace
{
    expected<network_settings_t> network_from_json(const json& config)
    {
        network_settings_t net;
        BST_ASSIGN(primary, find<json>(config, "primary"));
        assign_if<std::string>(primary, "interface_name", net, &network_settings_t::interface_name);
        assign_if<std::string>(primary, "interface_address", net, &network_settings_t::interface_address);

        return net;
    }

    video_info_t translate_sdp_video_settings(const video_sender_info_t video_settings)
    {
        video_info_t info;
        info.chroma_sub_sampling = video_settings.chroma_sub_sampling;
        info.width               = video_settings.width;
        info.height              = video_settings.height;
        return info;
    }

    expected<receiver_settings> translate_json(const json& config, sdp_settings_t sdp_settings)
    {
        receiver_settings s;
        BST_ASSIGN(network, find<json>(config, "network"));
        BST_CHECK_ASSIGN(s.primary, network_from_json(network));
        if(sdp_settings.primary.destination_ip.has_value())
        {
            s.primary.source_ip_address = sdp_settings.primary.destination_ip.value();
        }
        if(sdp_settings.primary.destination_port.has_value())
        {
            s.primary.source_port = static_cast<uint16_t>(sdp_settings.primary.destination_port.value());
        }

        BST_ASSIGN(capabilities, find<json>(config, "capabilities"));
        BST_ENFORCE(capabilities.is_array(), "capabilities is not an array");

        // TODO: Only checking the first position but should receive more capabilities in the future
        const auto c = capabilities[0];

        if(c == "video/raw" && std::holds_alternative<video_sender_info_t>(sdp_settings.format))
        {
            auto v   = std::get<video_sender_info_t>(sdp_settings.format);
            s.format = translate_sdp_video_settings(v);
        }
        else if(c == "audio/raw" && std::holds_alternative<audio_sender_info_t>(sdp_settings.format))
        {
            audio_info_t info;
            s.format = info;
        }
        else
        {
            BST_FAIL("invalid media type: {}", c);
        }

        return s;
    }

    template <typename In> expected<gst_receiver_plugin_uptr> do_create_plugin(const In&, const receiver_settings&)
    {
        BST_FAIL("invalid format");
    }

    expected<gst_receiver_plugin_uptr> do_create_plugin(const video_info_t& format, const receiver_settings& settings)
    {
        return create_gst_st2110_20_plugin(settings, format);
    }
} // namespace

expected<gst_receiver_plugin_uptr> plugins::create_gst_receiver_plugin(const std::string& config,
                                                                       const std::string& sdp) noexcept
{

    BST_ASSIGN(sdp_settings, parse_sdp(sdp));

    BST_ASSIGN(s, translate_json(json::parse(config), sdp_settings));

    return match(s.format, overload{[&](const auto& f) { return do_create_plugin(f, s); }});
}