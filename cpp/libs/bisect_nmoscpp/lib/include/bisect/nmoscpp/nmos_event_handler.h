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
#include <nmos/resources.h>

namespace bisect::nmoscpp
{
    struct sdp_info_t
    {
        uint8_t payload_type;
        float packet_time;
    };

    class nmos_event_handler_t
    {
      public:
        virtual ~nmos_event_handler_t() = default;

        [[nodiscard]] virtual bisect::expected<std::string>
        handle_active_state_changed(const nmos::resource& resource, const nmos::resource& connection_resource,
                                    const std::string& transport_params) = 0;

        [[nodiscard]] virtual maybe_ok handle_patch_request(const nmos::resource& resource,
                                                            const nmos::resource& connection_resource,
                                                            const std::string& endpoint_staged)               = 0;
        [[nodiscard]] virtual bisect::expected<sdp_info_t> handle_sdp_info_request(const nmos::id& sender_id) = 0;
    };

} // namespace bisect::nmoscpp
