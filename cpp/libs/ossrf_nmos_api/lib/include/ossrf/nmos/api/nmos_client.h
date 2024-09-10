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
#include <nmos/type.h>
#include <memory>
namespace ossrf
{
    class nmos_client_t;

    using nmos_client_uptr = std::unique_ptr<nmos_client_t>;

    class nmos_client_t
    {
      public:
        static bisect::expected<nmos_client_uptr> create(const std::string& node_id,
                                                         const std::string& node_configuration) noexcept;

        ~nmos_client_t();

        bisect::maybe_ok add_device(const std::string& config) noexcept;

        bisect::maybe_ok add_receiver(const std::string& device_id, const std::string& config,
                                      bisect::nmoscpp::receiver_activation_callback_t callback) noexcept;

        bisect::maybe_ok add_sender(const std::string& device_id, const std::string& config,
                                    bisect::nmoscpp::sender_activation_callback_t callback) noexcept;

        bisect::maybe_ok remove_receiver(const std::string& device_id, const std::string& config) noexcept;

        bisect::maybe_ok remove_sender(const std::string& device_id, const std::string& config) noexcept;

        bisect::maybe_ok remove_resource(const std::string& id, const nmos::type& type) noexcept;

      private:
        struct impl;
        std::unique_ptr<impl> impl_;
        nmos_client_t(std::unique_ptr<impl>&& i) noexcept;
    };
} // namespace ossrf
