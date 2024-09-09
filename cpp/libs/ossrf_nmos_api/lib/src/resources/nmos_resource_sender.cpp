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

#include "nmos_resource_sender.h"
#include "bisect/sdp/media_types.h"
#include "../serialization/sender.h"
#include <nmos/id.h>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;
using json = nlohmann::json;

nmos_resource_sender_t::nmos_resource_sender_t(const std::string& device_id, const nmos_sender_t& config,
                                               sender_activation_callback_t callback)
    : config_(config), activation_callback_(callback), device_id_(device_id)
{
}

const std::string& nmos_resource_sender_t::get_device_id() const
{
    return device_id_;
}

const std::string& nmos_resource_sender_t::get_id() const
{
    return config_.id;
}

maybe_ok nmos_resource_sender_t::handle_activation(bool master_enable, json& transport_params)
{

    // TODO: Resolve auto params
    activation_callback_(master_enable, transport_params);

    master_enable_ = master_enable;

    return {};
}

maybe_ok nmos_resource_sender_t::handle_patch(bool master_enable, const json& configuration)
{
    return {};
}
