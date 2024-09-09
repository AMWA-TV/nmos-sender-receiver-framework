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

#pragma once

#include "bisect/expected.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/json/json.h"

namespace ossrf
{
    bisect::expected<bisect::nmoscpp::network_t> network_from_json(const nlohmann::json& config, bool is_receiver);
    bisect::expected<bisect::nmoscpp::network_leg_t> network_leg_from_json(const nlohmann::json& config,
                                                                           bool is_receiver);
} // namespace ossrf
