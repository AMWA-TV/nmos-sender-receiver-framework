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

#include "nmos_resource_receiver.h"
#include "bisect/fmt.h"
#include <nlohmann/json.hpp>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;
using json = nlohmann::json;

namespace
{
    std::optional<std::string> get_sdp_from_configuration(const nlohmann::json& configuration)
    {
        const auto transport_file_it = configuration.find("transport_file");
        if(transport_file_it == configuration.end())
        {
            fmt::print("NMOS Receiver: No transport_file in receiver configuration!\n");
            return std::nullopt;
        }
        const auto& transport_file = *transport_file_it;

        const auto type_it = transport_file.find("type");
        if(type_it == transport_file.end())
        {
            fmt::print("NMOS Receiver: No type in receiver configuration!\n");
            return std::nullopt;
        }
        const auto& type = *type_it;
        if(!type.is_string())
        {
            fmt::print("NMOS Receiver: Type is not a string in receiver configuration!\n");
            return std::nullopt;
        }
        const auto type_value                  = type.get<std::string>();
        constexpr auto transport_file_type_sdp = "application/sdp";
        if(type_value != transport_file_type_sdp)
        {
            fmt::print("NMOS Receiver: Type is not application/sdp in receiver configuration!\n");
            return std::nullopt;
        }

        const auto data_it = transport_file.find("data");
        if(data_it == transport_file.end())
        {
            fmt::print("NMOS Receiver: No data in receiver configuration!\n");
            return std::nullopt;
        }
        const auto& data = *data_it;
        if(!data.is_string())
        {
            fmt::print("NMOS Receiver: Data is not a string in receiver configuration!\n");
            return std::nullopt;
        }
        const auto data_value = data.get<std::string>();
        return data;
    }

} // namespace
nmos_resource_receiver_t::nmos_resource_receiver_t(const std::string& device_id, const nmos_receiver_t& config,
                                                   receiver_activation_callback_t callback)
    : config_(config), activation_callback_(callback), device_id_(device_id)
{
}

const std::string& nmos_resource_receiver_t::get_device_id() const
{
    return device_id_;
}

const std::string& nmos_resource_receiver_t::get_id() const
{
    return config_.id;
}

maybe_ok nmos_resource_receiver_t::handle_patch(bool master_enable, const json& configuration)
{
    if(master_enable)
    {
        sdp_ = get_sdp_from_configuration(configuration);
    }
    else
    {
        sdp_ = std::nullopt;
    }

    return {};
}

maybe_ok nmos_resource_receiver_t::handle_activation(bool master_enable, json& transport_params)
{
    // TODO: Resolve auto params
    if(master_enable && sdp_.has_value())
    {
        fmt::print("receiver {}::handle_activation - callback with active SDP: {}\n", config_.id, sdp_.value());
    }
    else
    {
        fmt::print("receiver {}::handle_activation - callback with disabled\n", config_.id);
    }

    activation_callback_(sdp_, master_enable);

    master_enable_ = master_enable;

    return {};
}
