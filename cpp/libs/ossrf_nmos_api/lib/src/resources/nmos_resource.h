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
#include <nlohmann/json_fwd.hpp>
#include <nmos/id.h>
#include <string>
#include <memory>
#include <nmos/type.h>

namespace ossrf
{
    class nmos_resource_t
    {
      public:
        virtual ~nmos_resource_t() = default;

        [[nodiscard]] virtual bisect::maybe_ok handle_patch(bool master_enable,
                                                            const nlohmann::json& configuration)   = 0;
        [[nodiscard]] virtual bisect::maybe_ok handle_activation(bool master_enable,
                                                                 nlohmann::json& transport_params) = 0;

        [[nodiscard]] virtual const std::string& get_id() const = 0;

        [[nodiscard]] virtual const std::string& get_device_id() const = 0;

        [[nodiscard]] virtual nmos::type get_resource_type() const = 0;
    };

    using nmos_resource_ptr  = std::shared_ptr<nmos_resource_t>;
    using nmos_resource_uptr = std::unique_ptr<nmos_resource_t>;
} // namespace ossrf
