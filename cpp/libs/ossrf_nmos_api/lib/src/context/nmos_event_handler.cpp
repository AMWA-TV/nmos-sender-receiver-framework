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

#include "nmos_event_handler.h"
#include "bisect/expected.h"
#include "bisect/expected/macros.h"
#include <nlohmann/json.hpp>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;

using json = nlohmann::json;

nmos_event_handler::nmos_event_handler(nmos_context_ptr context) : context_(context)
{
}

expected<std::string> nmos_event_handler::handle_active_state_changed(const nmos::resource& resource,
                                                                      const nmos::resource& connection_resource,
                                                                      const std::string& transport_params)
{
    const auto master_enable =
        connection_resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::master_enable).as_bool();

    BST_ASSIGN(r, context_->resources().find_resource(resource.id));
    auto tp = json::parse(transport_params);
    // TODO: Check if there are any auto params and return the resolved params
    BST_CHECK(r->handle_activation(master_enable, tp));

    return transport_params;
}

maybe_ok nmos_event_handler::handle_patch_request(const nmos::resource& resource,
                                                  const nmos::resource& connection_resource,
                                                  const std::string& endpoint_staged)
{
    fmt::print("handle_patch_request: {} {} {}", utility::us2s(resource.id), utility::us2s(connection_resource.id),
               endpoint_staged);

    const auto master_enable =
        connection_resource.data.at(nmos::fields::endpoint_staged).at(nmos::fields::master_enable).as_bool();

    BST_ASSIGN(r, context_->resources().find_resource(resource.id));
    BST_CHECK(r->handle_patch(master_enable, json::parse(endpoint_staged)));
    return {};
}
