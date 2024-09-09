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

#include "context.h"

using namespace ossrf;

nmos_context::nmos_context(const std::string& node_id)
{
    resources_ = std::make_unique<resource_map_t>();
    nmos_api_  = nmos_impl::create(node_id);
}

nmos_t& nmos_context::nmos()
{
    return *nmos_api_;
}

resource_map_t& nmos_context::resources()
{
    return *resources_;
}
