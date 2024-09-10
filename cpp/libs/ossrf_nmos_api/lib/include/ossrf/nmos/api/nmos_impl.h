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
#include "ossrf/nmos/api/nmos.h"
#include <nmos/type.h>

namespace ossrf
{
    class nmos_impl : public nmos_t
    {

      public:
        static nmos_uptr create(const std::string& node_id);

        ~nmos_impl() override;

        [[nodiscard]] bisect::maybe_ok
        add_node(const std::string& node_configuration,
                 bisect::nmoscpp::nmos_event_handler_t* nmos_event_handler) noexcept override;

        [[nodiscard]] bisect::maybe_ok add_device(const bisect::nmoscpp::nmos_device_t& config) noexcept override;

        [[nodiscard]] bisect::maybe_ok add_receiver(const std::string& device_id,
                                                    const bisect::nmoscpp::nmos_receiver_t& config) noexcept override;

        [[nodiscard]] bisect::maybe_ok add_sender(const std::string& device_id,
                                                  const bisect::nmoscpp::nmos_sender_t& config) noexcept override;

        [[nodiscard]] bisect::maybe_ok modify_device(const bisect::nmoscpp::nmos_device_t& config) noexcept override;

        [[nodiscard]] bisect::maybe_ok
        modify_device_sub_resources(const std::string& device_id, const std::vector<std::string>& receivers,
                                    const std::vector<std::string>& senders) noexcept override;

        [[nodiscard]] bisect::maybe_ok
        modify_receiver(const std::string& device_id, const bisect::nmoscpp::nmos_receiver_t& config) noexcept override;

        [[nodiscard]] bisect::maybe_ok modify_sender(const std::string& device_id,
                                                     const bisect::nmoscpp::nmos_sender_t& config) noexcept override;

        [[nodiscard]] bisect::maybe_ok remove_resource(const std::string& resource_id,
                                                       const nmos::type& type) noexcept override;

        [[nodiscard]] bisect::maybe_ok update_clocks(const std::string& clocks) noexcept override;

      private:
        struct impl;
        std::unique_ptr<impl> impl_;

        nmos_impl(std::unique_ptr<impl>&& i) noexcept;
    };
}; // namespace ossrf
