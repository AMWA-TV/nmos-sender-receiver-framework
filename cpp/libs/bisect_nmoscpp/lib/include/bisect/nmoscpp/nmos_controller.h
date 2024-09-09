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

#include "bisect/nmoscpp/detail/internal.h"
#include "bisect/nmoscpp/base_nmos_controller.h"
#include "bisect/nmoscpp/logger.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected.h"
#include <nmos/capabilities.h>
#include <nmos/interlace_mode.h>
#include <nmos/node_resources.h>
#include <nmos/transport.h>
#include <nmos/connection_resources.h>
#include <nmos/connection_activation.h>
#include <nmos/node_interfaces.h>
#include <memory>

namespace bisect::nmoscpp
{
    class nmos_controller_t
    {
      public:
        nmos_controller_t(logger_t& log, web::json::value configuration, nmos_event_handler_t* event_handler);

        using opt_json = std::optional<web::json::value>;

        struct options_t
        {
            opt_json interfaces;
            opt_json clocks;
        };

        nmos::resource make_node(const nmos::id& node_id, options_t options);
        nmos::resource make_device(const nmos_device_t& device_config, const std::vector<nmos::id>& receivers_ids,
                                   const std::vector<nmos::id>& senders_ids);

        nmos::resource make_receiver(const nmos::id& device_id, const nmos_receiver_t& config);
        nmos::resource make_connection_receiver(const nmos::id& device_id, const nmos_receiver_t& config);

        nmos::resource make_sender(const nmos::id& device_id, const nmos_sender_t& sender_config);
        nmos::resource make_connection_sender(const nmos::id& device_id, const nmos_sender_t& sender_config);

        bisect::expected<nmos::resource> make_source(const nmos::id& device_id, const nmos_sender_t& sender);

        bisect::expected<nmos::resource> make_audio_flow(const nmos::id& device_id, const nmos::id& source_id,
                                                         const flow_t& flow_config, const audio_sender_info_t& media);
        bisect::expected<nmos::resource> make_video_flow(const nmos::id& device_id, const nmos::id& source_id,
                                                         const flow_t& flow_config, std::string media_type,
                                                         const video_sender_info_t& media);

        bisect::maybe_ok insert_resource(nmos::resource&& resource);
        bisect::maybe_ok insert_connection_resource(nmos::resource&& resource);
        bisect::maybe_ok modify_resource(const nmos::id& resource_id, std::function<void(nmos::resource&)> modifier);
        bisect::maybe_ok modify_connection_resource(const nmos::id& resource_id,
                                                    std::function<void(nmos::resource&)> modifier);
        [[nodiscard]] bisect::maybe_ok modify_connection_receiver(const nmos_receiver_t& config);

        bisect::expected<nmos::resource> find_resource(const nmos::id& id);
        bisect::expected<nmos::resource> find_connection_resource(const nmos::id& id);

        maybe_ok erase_resource(const nmos::id& resource_id);
        maybe_ok erase_connection_resource(const nmos::id& resource_id);
        maybe_ok erase_device(const nmos::id& device_id);

        [[nodiscard]] maybe_ok update_transport_file(const nmos::id& sender_id);
        bool has_resource(const nmos::id& id, const nmos::type& type);
        std::vector<utility::string_t> get_interfaces_names(const nmos::settings& settings, bool smpte2022_7);
        bisect::maybe_ok call_senders_with(const nmos::id& node_id, std::function<bisect::maybe_ok(nmos::resource&)> f);

        void open();
        void close();

      private:
        nmos_base_controller_t base_controller_;
        nmos::server server_;
        nmos::connection_resource_auto_resolver resolve_auto_;
        std::function<maybe_ok(unsigned int milliseconds, nmos::resources& resources, nmos::resource&& resource)>
            insert_resource_after_;

        std::function<maybe_ok(unsigned int milliseconds, nmos::resources& resources, const nmos::id& id,
                               std::function<void(nmos::resource&)> modifier)>
            modify_resource_after_;

        std::function<maybe_ok(unsigned int milliseconds, nmos::resources& resources, const nmos::id& resource_id)>
            erase_resource_after_;
    };

    using nmos_controller_uptr = std::unique_ptr<nmos_controller_t>;
} // namespace bisect::nmoscpp
