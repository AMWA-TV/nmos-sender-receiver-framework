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

#include "serialization/sender.h"
#include "serialization/network.h"
#include "serialization/video.h"
#include "serialization/meta.h"
#include "serialization/media_types.h"
#include "utils.h"
#include "bisect/expected/macros.h"
#include "bisect/sdp/builder.h"
#include <nmos/id.h>

using namespace ossrf;
using namespace ossrf::media_types;
using namespace bisect;
using namespace bisect::nmoscpp;

using json = nlohmann::json;

namespace
{
    expected<nmos::interlace_mode> to_interlace_mode(const std::string& s)
    {
        if(s == "progressive") return nmos::interlace_modes::progressive;
        if(s == "interlaced_tff") return nmos::interlace_modes::interlaced_tff;
        if(s == "interlaced_bff") return nmos::interlace_modes::interlaced_bff;
        if(s == "interlaced_psf") return nmos::interlace_modes::interlaced_psf;
        BST_FAIL("invalid frame structure '{}'", s);
    }

    expected<video_sender_info_t> video_sender_info_from_json(const json& media)
    {
        video_sender_info_t info;
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

    expected<audio_sender_info_t> audio_l24_sender_info_from_json(const json& media)
    {
        audio_sender_info_t info;
        BST_CHECK_ASSIGN(info.number_of_channels, find<int>(media, "number_of_channels"));
        BST_CHECK_ASSIGN(info.sampling_rate, find<int>(media, "sampling_rate"));
        BST_CHECK_ASSIGN(info.packet_time, find<float>(media, "packet_time"));
        info.bits_per_sample = 24;
        return info;
    }

    std::string synthetize_id(std::string sender_id, int delta)
    {
        const auto last_digit_value = std::stoi(sender_id.substr(sender_id.size() - 1), 0, 16);
        int new_last_digit_value    = (last_digit_value + delta) % 16;
        auto new_id                 = sender_id;
        sprintf(new_id.data() + sender_id.size() - 1, "%x", new_last_digit_value);
        return new_id;
    }

    void get_extra(const json& j, std::string_view name, web::json::value& target)
    {
        const auto it = j.find(name);
        if(it != j.end())
        {
            target = web::json::value::parse(utility::s2us(it->dump()));
        }
    }

} // namespace

expected<nmos_sender_t> ossrf::nmos_sender_from_json(const json& config)
{
    nmos_sender_t sender{};

    nmos_meta_from_json(config, sender);
    assign_if_or_value<std::string>(config, "protocol", sender, &nmos_sender_t::protocol,
                                    "urn:x-nmos:transport:rtp.mcast");

    const auto flow_id   = synthetize_id(sender.id, 1);
    const auto source_id = synthetize_id(sender.id, 2);

    assign_if_or_value<bool>(config, "master_enable", sender, &nmos_sender_t::master_enable, true);

    BST_ASSIGN(network, find<json>(config, "network"));
    BST_CHECK_ASSIGN(sender.network, network_from_json(network, false));
    BST_CHECK_ASSIGN(sender.media_type, find<std::string>(config, "media_type"));
    sender.source.id          = source_id;
    sender.source.label       = sender.label;
    sender.source.description = sender.description;
    sender.flow.id            = flow_id;
    sender.flow.label         = sender.label;
    sender.flow.description   = sender.description;
    BST_CHECK_ASSIGN(sender.payload_type, maybe_find<uint8_t>(config, "payload_type"));

    BST_ASSIGN(media, find<json>(config, "media"));

    if(sender.media_type == media_types::VIDEO_RAW)
    {
        BST_ASSIGN(info, video_sender_info_from_json(media));
        sender.media      = info;
        sender.format     = nmos::formats::video;
        sender.grain_rate = info.exact_framerate;
    }
    else if(sender.media_type == media_types::AUDIO_L24)
    {
        BST_ASSIGN(info, audio_l24_sender_info_from_json(media));
        sender.media      = info;
        sender.format     = nmos::formats::audio;
        sender.grain_rate = info.sampling_rate;
    }
    else
    {
        BST_FAIL("invalid media type: {}", sender.media_type);
    }

    BST_ASSIGN(maybe_sdp, find_or<std::string>(config, "sdp"));

    if(!maybe_sdp.empty())
    {
        sender.forced_sdp = maybe_sdp;
    }

    get_extra(config, "sender", sender.extra);
    get_extra(config, "flow", sender.flow.extra);

    return sender;
}
