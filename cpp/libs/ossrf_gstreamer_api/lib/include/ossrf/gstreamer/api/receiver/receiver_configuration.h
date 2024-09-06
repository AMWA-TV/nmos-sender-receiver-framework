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