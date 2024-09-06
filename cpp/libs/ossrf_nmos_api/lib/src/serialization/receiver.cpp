#include "serialization/receiver.h"
#include "serialization/network.h"
#include "serialization/meta.h"
#include "utils.h"
#include "bisect/expected/helpers.h"
#include "bisect/expected/macros.h"

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;

using nlohmann::json;

namespace
{
    maybe_ok format_specific(const json& config, nmos_receiver_t& receiver)
    {
        BST_ASSIGN(capabilities, find<json>(config, "capabilities"));

        BST_ENFORCE(capabilities.is_array(), "Receiver capabilities is not an array");
        auto c = capabilities.get<std::vector<std::string>>();
        BST_ENFORCE(c.size() == 1, "Only support one receiver capabilitiy");

        auto media_type = c[0];
        if(media_type == "video/raw")
        {
            receiver.media_types = {nmos::media_types::video_raw};
            receiver.format      = nmos::formats::video;
        }
        else if(media_type == "audio/raw")
        {
            receiver.media_types = {nmos::media_types::audio_L24};
            receiver.format      = nmos::formats::audio;
        }
        else
        {
            BST_FAIL("cap {} not supported", media_type);
        }

        return {};
    }

} // namespace

expected<nmos_receiver_t> ossrf::nmos_receiver_from_json(const json& config)
{
    nmos_receiver_t receiver{};
    nmos_meta_from_json(config, receiver);
    assign_if_or_value<bool>(config, "master_enable", receiver, &nmos_receiver_t::master_enable, true);
    assign_if<std::string>(config, "sender_id", receiver, &nmos_receiver_t::sender_id);
    assign_if_or_value<std::string>(config, "protocol", receiver, &nmos_receiver_t::protocol,
                                    "urn:x-nmos:transport:rtp.mcast");

    BST_ASSIGN(network, find<json>(config, "network"));
    BST_CHECK_ASSIGN(receiver.network, network_from_json(network, true));
    BST_CHECK(format_specific(config, receiver));

    auto transport_file_it = config.find("transport_file");
    if(transport_file_it != config.end())
    {
        BST_ASSIGN(data, find<std::string>(*transport_file_it, "data"));
        BST_ASSIGN(type, find<std::string>(*transport_file_it, "type"));
        BST_ENFORCE(type == "application/sdp", "Sender transport file is not an SDP");
        receiver.sdp_data = data;
    }

    return receiver;
}
