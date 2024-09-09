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

#include "serialization/video.h"
#include "bisect/expected/macros.h"

using namespace bisect;

expected<nmos::rational> ossrf::framerate_from_json(const nlohmann::json& config)
{
    BST_ASSIGN(numerator, find<int64_t>(config, "num"));
    BST_ASSIGN(denominator, find<int64_t>(config, "den"));
    return nmos::rational{numerator, denominator};
}
