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

#include "serialization/network.h"
#include "bisect/expected/helpers.h"

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;

using json = nlohmann::json;

expected<network_t> ossrf::network_from_json(const json& config, bool is_receiver)
{
    network_t net;
    BST_ASSIGN(primary, find<json>(config, "primary"));
    BST_CHECK_ASSIGN(net.primary, network_leg_from_json(primary, is_receiver));

    auto secondary = config.find("secondary");
    if(secondary != config.end())
    {
        BST_CHECK_ASSIGN(net.secondary, network_leg_from_json(*secondary, is_receiver));
    }

    return net;
}

expected<network_leg_t> ossrf::network_leg_from_json(const json& config, bool is_receiver)
{
    network_leg_t leg;
    assign_if_or_value<bool>(config, "rtp_enabled", leg, &network_leg_t::rtp_enabled, true);
    assign_if<std::string>(config, "source_address", leg, &network_leg_t::source_ip);
    assign_if<int>(config, "destination_port", leg, &network_leg_t::destination_port);
    assign_if<std::string>(config, "interface_name", leg, &network_leg_t::interface_name);

    if(is_receiver)
    {
        assign_if<std::string>(config, "multicast_address", leg, &network_leg_t::destination_ip);
        assign_if<std::string>(config, "interface_address", leg, &network_leg_t::interface_ip);
    }
    else
    {
        assign_if<std::string>(config, "destination_address", leg, &network_leg_t::destination_ip);
        assign_if<int>(config, "source_port", leg, &network_leg_t::source_port);
    }

    return leg;
}
