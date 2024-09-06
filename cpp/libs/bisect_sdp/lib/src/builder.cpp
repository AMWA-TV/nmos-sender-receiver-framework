#include "bisect/sdp/builder.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected/match.h"
#include "bisect/sdp/clocks.h"
#include "fmt/format.h"
#include <chrono>

using namespace bisect;

namespace
{
    std::string serialize_atribute(const sdp::mediaclk_t& mediaclk)
    {
        return fmt::format("a=mediaclk:{}",
                           match(mediaclk, overload{[&](const auto& s) { return sdp::to_string(s); }}));
    }

    std::string serialize_atribute(const sdp::refclk_t& refclk)
    {
        return fmt::format("a=ts-refclk:{}", match(refclk, overload{[&](const auto& s) { return sdp::to_string(s); }}));
    }

    std::string build_header(const sdp::sdp_settings_t& f)
    {

        const auto source_ip = f.primary.source_ip.has_value() ? *f.primary.source_ip : " ";

        constexpr auto t = R"(v=0
o=- {sess_id} {sess_version} IN IP4 {source_ip}
s={description}
t=0 0
a=recvonly
)";

        return fmt::format(t, fmt::arg("description", f.origin.description), fmt::arg("sess_id", f.origin.session_id),
                           fmt::arg("sess_version", f.origin.session_version), fmt::arg("source_ip", source_ip));
    }

    std::string build_clock_info(const sdp::sdp_settings_t& f)
    {
        constexpr auto clock =
            R"({}
{}
)";
        return fmt::format(clock, serialize_atribute(f.mediaclk), serialize_atribute(f.ts_refclk));
    }

    std::string build_connection(const nmoscpp::network_leg_t& net)
    {
        constexpr auto t = R"(c=IN IP4 {destination_ip}/128
a=source-filter: incl IN IP4 {destination_ip} {source_ip})";

        return fmt::format(
            t, fmt::arg("source_ip", net.source_ip.has_value() ? net.source_ip.value() : ""),
            fmt::arg("destination_ip", net.destination_ip.has_value() ? net.destination_ip.value() : ""));
    }

    expected<std::string> get_video_media_type()
    {
        return "raw";
    }

    expected<std::string> build_media(const nmoscpp::network_leg_t& net, const sdp::rtp_settings_t& rtp,
                                      const nmoscpp::video_sender_info_t& f)
    {
        BST_ASSIGN(media_type, get_video_media_type());

        const auto destination_port = net.destination_port.has_value() ? *net.destination_port : -1;

        constexpr auto t =
            R"(m=video {destination_port} RTP/AVP {payload_type}
{connection}
a=rtpmap:{payload_type} {media_type}/90000
a=fmtp:{payload_type} sampling=YCbCr-4:2:2; width={width}; height={height}; exactframerate={frame_rate};{interlace} depth=10; colorimetry={colorimetry}; PM=2110GPM; SSN=ST2110-20:2017;
)";

        const auto frame_rate_s = fmt::format("{}/{}", f.exact_framerate.numerator(), f.exact_framerate.denominator());
        const auto interlace    = f.structure == nmos::interlace_modes::progressive ? "progressive" : "interlaced";

        return fmt::format(t, fmt::arg("media_type", media_type), fmt::arg("payload_type", rtp.payload_type),
                           fmt::arg("destination_port", destination_port), fmt::arg("width", f.width),
                           fmt::arg("height", f.height), fmt::arg("frame_rate", frame_rate_s),
                           fmt::arg("interlace", interlace), fmt::arg("colorimetry", f.chroma_sub_sampling),
                           fmt::arg("connection", build_connection(net)));
    }

    expected<std::string> build_media(const nmoscpp::network_leg_t& net, const sdp::rtp_settings_t& rtp,
                                      const nmoscpp::audio_sender_info_t& f)
    {
        constexpr auto t = R"(m=audio {destination_port} RTP/AVP {payload_type}
a=rtpmap:{payload_type} {type}/{sample_rate}/{channel_count}
a=fmtp:{payload_type} channel-order={channel_order};
a=ptime:{ptime}
a=maxptime:{ptime}
)";

        const auto destination_port = net.destination_port.has_value() ? *net.destination_port : -1;
        // const auto type     = f.is_am824 ? "AM824" : fmt::format("L{}", as_bit_count(f.sample_format));
        const auto type     = fmt::format("L{}", f.bits_per_sample);
        const auto ptime_us = static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<float>(f.packet_time)).count());
        const auto ptime = fmt::format("{:.3f}", ptime_us / 1000.0);

        return fmt::format(t, fmt::arg("payload_type", rtp.payload_type), fmt::arg("channel_order", "SMPTE2110.(U02)"),
                           fmt::arg("destination_port", destination_port), fmt::arg("type", type),
                           fmt::arg("sample_rate", f.sampling_rate), fmt::arg("channel_count", f.number_of_channels),
                           fmt::arg("ptime", ptime));
    }
} // namespace

expected<std::string> sdp::build_sdp(const sdp_settings_t& config)
{
    BST_ASSIGN(primary_media,
               match(config.format, [&config](const auto& f) { return build_media(config.primary, config.rtp, f); }));

    if(config.secondary.has_value())
    {
        BST_ASSIGN(secondary_media, match(config.format, [&config](const auto& f) {
                       return build_media(config.secondary.value(), config.rtp, f);
                   }));
        constexpr auto group         = "a=group:DUP primary secondary\n";
        constexpr auto primary_mid   = "a=mid:primary\n";
        constexpr auto secondary_mid = "a=mid:secondary\n";

        constexpr auto t =
            R"({header}{clock_info}{group}{primary_media}{primary_mid}{secondary_media}{clock_info}{secondary_mid})";
        return fmt::format(t, fmt::arg("header", build_header(config)), fmt::arg("group", group), // done
                           fmt::arg("primary_media", primary_media), fmt::arg("primary_mid", primary_mid),
                           fmt::arg("secondary_media", secondary_media), fmt::arg("secondary_mid", secondary_mid),
                           fmt::arg("clock_info", build_clock_info(config)));
    }
    else
    {
        constexpr auto t = R"({header}{clock_info}{media})";
        return fmt::format(t, fmt::arg("header", build_header(config)), fmt::arg("media", primary_media),
                           fmt::arg("clock_info", build_clock_info(config)));
    }
}
