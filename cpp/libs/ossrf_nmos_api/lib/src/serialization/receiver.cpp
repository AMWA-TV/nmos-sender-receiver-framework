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
