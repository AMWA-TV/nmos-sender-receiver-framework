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

#include <sdp/sdp.h>
//////////////////////////////////////////////////////////////////////////////
// Work around a missing forward declaration in cpprest
#include <cpprest/json.h>

namespace web
{
    namespace json
    {
        bool operator<(const web::json::value& lhs, const web::json::value& rhs);
    }
} // namespace web

#include <cpprest/json_ops.h>
//////////////////////////////////////////////////////////////////////////////
#include "bisect/sdp/reader.h"
#if !defined(U)
#define U(X) _XPLATSTR(X)
#endif
#include <nmos/sdp_utils.h>
#undef U
#include <sdp/json.h>
#include "bisect/expected/macros.h"
#include "bisect/expected/helpers.h"
#include "bisect/sdp/clocks.h"
#include "bisect/json.h"
#include "bisect/nmoscpp/configuration.h"

using namespace bisect;
using namespace bisect::sdp;
using namespace bisect::selectors;
using namespace bisect::nmoscpp;

namespace
{
    expected<refclk_t> to_refclk_t(const std::vector<nmos::sdp_parameters::ts_refclk_t>& params)
    {
        BST_ENFORCE(params.size() == 1, "params size is different from 1");
        if(params[0].clock_source.name == utility::conversions::to_string_t("localmac"))
        {
            BST_ASSIGN(mac_address,
                       ethernet::to_mac_address(utility::conversions::to_utf8string(params[0].mac_address)));

            return refclks::localmac_t{.address = mac_address};
        }
        else if(params[0].clock_source.name == utility::conversions::to_string_t("ptp"))
        {
            auto ptp_server = utility::conversions::to_utf8string(params[0].ptp_server);
            auto pos        = ptp_server.find(":");
            if(pos == std::string::npos)
            {
                return refclks::ptp_t{.gmid = utility::conversions::to_utf8string(ptp_server), .domain = std::nullopt};
            }
            return refclks::ptp_t{.gmid   = utility::conversions::to_utf8string(ptp_server.substr(0, pos)),
                                  .domain = static_cast<uint8_t>(std::stoi(ptp_server.substr(pos + 1)))};
        }
        BST_FAIL("Reference clock {} is invalid.", utility::conversions::to_utf8string(params[0].clock_source.name));
    }

    expected<mediaclk_t> to_mediaclk_t(const nmos::sdp_parameters::mediaclk_t& params)
    {
        if(params.clock_source.name == utility::conversions::to_string_t("direct"))
        {
            return mediaclks::direct_t{.offset = static_cast<uint64_t>(std::stoi(params.clock_parameters))};
        }
        else if(params.clock_source.name == utility::conversions::to_string_t("sender"))
        {
            return mediaclks::sender_t{};
        }
        BST_FAIL("Media clock {} is invalid.", utility::conversions::to_utf8string(params.clock_source.name));
    }

    expected<network_leg_t> to_network_settings_t(const nlohmann::json& sdp_settings)
    {
        auto s = network_leg_t{};
        BST_CHECK_ASSIGN(s.destination_port, select<uint16_t>(sdp_settings, element("media_descriptions"), index(0),
                                                              element("media"), element("port")));

        BST_CHECK_ASSIGN(s.destination_ip, select<std::string>(sdp_settings, element("media_descriptions"), index(0),
                                                               element("attributes"), name_value("source-filter"),
                                                               element("destination_address")));

        BST_CHECK_ASSIGN(s.source_ip, select<std::string>(sdp_settings, element("media_descriptions"), index(0),
                                                          element("attributes"), name_value("source-filter"),
                                                          element("source_addresses"), index(0)));

        s.interface_ip = "0.0.0.0";

        return s;
    }

    expected<audio_sender_info_t> get_raw_audio_params(const nmos::sdp_parameters& sdp_params,
                                                       const utility::string_t& encoding_name)
    {
        const auto params            = nmos::get_audio_L_parameters(sdp_params);
        unsigned int bits_per_sample = 0;
        if(encoding_name == utility::conversions::to_string_t("L16"))
        {
            bits_per_sample = 16;
        }
        else if(encoding_name == utility::conversions::to_string_t("L24"))
        {
            bits_per_sample = 24;
        }
        else
        {
            BST_FAIL("error parsing SDP: media type audio and encoding {} not supported",
                     utility::conversions::to_utf8string(encoding_name));
        }

        return audio_sender_info_t{
            .number_of_channels = static_cast<int>(params.channel_count),
            .bits_per_sample    = bits_per_sample,
            .sampling_rate      = static_cast<int>(params.sample_rate),
            .packet_time        = static_cast<float>(params.packet_time),
        };
    }

    expected<video_sender_info_t> get_raw_video_params(const nmos::sdp_parameters& sdp_params)
    {
        const auto params = nmos::get_video_raw_parameters(sdp_params);

        if(params.width == 0 || params.height == 0)
        {
            BST_FAIL("SDP reader: Invalid width and/or height.");
        }

        return video_sender_info_t{
            .height          = static_cast<int>(params.height),
            .width           = static_cast<int>(params.width),
            .exact_framerate = nmos::rational(params.exactframerate.numerator(), params.exactframerate.denominator()),
            .chroma_sub_sampling = "YCbCr-4:2:2",
            .structure = params.interlace ? nmos::interlace_modes::interlaced_tff : nmos::interlace_modes::progressive,
        };
    }
} // namespace

// TODO: US_5727 - having more than one media type per sdp file
// TODO: US_5728 - parsing secondary/redundant network case it exists
expected<sdp_settings_t> bisect::sdp::parse_sdp(const std::string& sdp)
{
    try
    {
        const auto parsed         = ::sdp::parse_session_description(sdp);
        const auto sdp_parameters = nmos::get_session_description_sdp_parameters(parsed);
        BST_ASSIGN(json_sdp, parse_json(utility::conversions::to_utf8string(parsed.serialize())));

        auto s                   = sdp_settings_t{};
        s.rtp.payload_type       = static_cast<uint8_t>(sdp_parameters.rtpmap.payload_type);
        s.origin.session_id      = std::to_string(sdp_parameters.origin.session_id);
        s.origin.session_version = std::to_string(sdp_parameters.origin.session_version);
        s.origin.description     = utility::conversions::to_utf8string(sdp_parameters.session_name);
        BST_CHECK_ASSIGN(s.mediaclk, to_mediaclk_t(sdp_parameters.mediaclk));
        BST_CHECK_ASSIGN(s.ts_refclk, to_refclk_t(sdp_parameters.ts_refclk));
        BST_CHECK_ASSIGN(s.primary, to_network_settings_t(json_sdp));

        if(sdp_parameters.media_type.name == utility::conversions::to_string_t("video") &&
           sdp_parameters.rtpmap.encoding_name == utility::conversions::to_string_t("raw"))
        {
            BST_ASSIGN(video, get_raw_video_params(sdp_parameters));

            s.format = video;
            return s;
        }

        if(sdp_parameters.media_type.name == utility::conversions::to_string_t("audio"))
        {
            const auto params = nmos::get_audio_L_parameters(sdp_parameters);

            BST_ASSIGN(audio, get_raw_audio_params(sdp_parameters, sdp_parameters.rtpmap.encoding_name));

            s.format = audio;

            return s;
        }

        BST_FAIL("error parsing SDP: media type {} and encoding {} not supported",
                 utility::conversions::to_utf8string(sdp_parameters.media_type.name),
                 utility::conversions::to_utf8string(sdp_parameters.rtpmap.encoding_name));
    }
    catch(std::exception& ex)
    {
        BST_FAIL("error parsing SDP: {}", ex.what());
    }
}