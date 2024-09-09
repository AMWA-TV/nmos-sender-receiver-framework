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

#include "serialization/device.h"
#include "serialization/receiver.h"
#include "serialization/sender.h"
#include "bisect/expected/helpers.h"
#include "bisect/json/json.h"
#include <nmos/id.h>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;

using nlohmann::json;

expected<nmos_device_t> ossrf::nmos_device_from_json(const nmos::id node_id, const nlohmann::json& device_config)
{
    nmos_device_t device{};
    device.node_id = node_id;

    BST_CHECK_ASSIGN(device.id, find<std::string>(device_config, "id"));
    BST_CHECK_ASSIGN(device.label, find<std::string>(device_config, "label"));
    BST_CHECK_ASSIGN(device.description, find<std::string>(device_config, "description"));

    return device;
}
