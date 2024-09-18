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

#include "bisect/expected.h"
#include "bisect/nmoscpp/nmos_event_handler.h"
#include "context.h"

namespace ossrf
{
    class nmos_event_handler : public bisect::nmoscpp::nmos_event_handler_t
    {
      public:
        nmos_event_handler(nmos_context_ptr context_);

        [[nodiscard]] bisect::expected<std::string>
        handle_active_state_changed(const nmos::resource& resource, const nmos::resource& connection_resource,
                                    const std::string& transport_params) override;

        [[nodiscard]] bisect::maybe_ok handle_patch_request(const nmos::resource& resource,
                                                            const nmos::resource& connection_resource,
                                                            const std::string& endpoint_staged) override;

        [[nodiscard]] bisect::expected<bisect::nmoscpp::sdp_info_t>
        handle_sdp_info_request(const nmos::id& resource_id) override;

      private:
        nmos_context_ptr const context_;
    };
} // namespace ossrf
