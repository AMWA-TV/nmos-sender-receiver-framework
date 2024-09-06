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
