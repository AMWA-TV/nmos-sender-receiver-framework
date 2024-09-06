#pragma once
#include <string>
#include <optional>
#include <variant>

namespace ossrf::gst::sender
{

    struct network_settings_t
    {
        std::optional<std::string> source_ip_address;
        std::string interface_name;
        std::string destination_ip_address;
        uint16_t destination_port;
    };

    enum class frame_structure_t
    {
        progressive,
        interlaced_tff,
        interlaced_bff,
        psf,
    };

    struct frame_rate_t
    {
        uint32_t num;
        uint32_t den;
    };

    struct video_info_t
    {
        int height;
        int width;
        frame_rate_t exact_framerate;
        std::string chroma_sub_sampling;
        frame_structure_t structure;
    };

    struct audio_info_t
    {
        int number_of_channels;
        unsigned int bits_per_sample;
        int sampling_rate;
        float packet_time;
    };

    using format_t = std::variant<video_info_t, audio_info_t>;

    struct sender_settings
    {
        format_t format;
        network_settings_t primary;
        std::optional<network_settings_t> secondary;
    };

} // namespace ossrf::gst::sender