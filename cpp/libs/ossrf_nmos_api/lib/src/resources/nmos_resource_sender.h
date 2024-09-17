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

#include "bisect/expected/macros.h"
#include "nmos_resource.h"
#include "bisect/nmoscpp/configuration.h"
#include <functional>
#include <optional>

namespace ossrf
{
    class nmos_resource_sender_t : public nmos_resource_t
    {
      public:
        nmos_resource_sender_t(const std::string& device_id, const bisect::nmoscpp::nmos_sender_t& config,
                               bisect::nmoscpp::sender_activation_callback_t callback);

        bisect::maybe_ok handle_activation(bool master_enable, nlohmann::json& transport_params) override;
        bisect::maybe_ok handle_patch(bool master_enable, const nlohmann::json& configuration) override;

        bisect::expected<bisect::nmoscpp::sdp_info_t> handle_sdp_info_request() override;

        const std::string& get_id() const override;

        const std::string& get_device_id() const override;

        nmos::type get_resource_type() const override;

      private:
        const bisect::nmoscpp::nmos_sender_t config_;
        bisect::nmoscpp::sender_activation_callback_t activation_callback_;
        bool master_enable_ = true;
        std::optional<std::string> sdp_;
        std::string device_id_;
    };

    using nmos_sender_ptr = std::shared_ptr<nmos_resource_sender_t>;
} // namespace ossrf
