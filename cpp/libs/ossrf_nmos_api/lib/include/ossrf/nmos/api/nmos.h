#pragma once
#include "bisect/nmoscpp/nmos_event_handler.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected.h"
#include <nmos/id.h>
#include <nmos/type.h>

namespace ossrf
{
    class nmos_t
    {
      public:
        virtual ~nmos_t() = default;

        [[nodiscard]] virtual bisect::maybe_ok add_node(const std::string& node_configuration,
                                                        bisect::nmoscpp::nmos_event_handler_t* nmos_event_handler) = 0;

        [[nodiscard]] virtual bisect::maybe_ok add_device(const bisect::nmoscpp::nmos_device_t& config) = 0;

        [[nodiscard]] virtual bisect::maybe_ok add_receiver(const std::string& device_id,
                                                            const bisect::nmoscpp::nmos_receiver_t& config) = 0;

        [[nodiscard]] virtual bisect::maybe_ok add_sender(const std::string& device_id,
                                                          const bisect::nmoscpp::nmos_sender_t& config) = 0;

        [[nodiscard]] virtual bisect::maybe_ok modify_device(const bisect::nmoscpp::nmos_device_t& device) = 0;

        [[nodiscard]] virtual bisect::maybe_ok modify_receiver(const std::string& device_id,
                                                               const bisect::nmoscpp::nmos_receiver_t& config) = 0;

        [[nodiscard]] virtual bisect::maybe_ok modify_sender(const std::string& device_id,
                                                             const bisect::nmoscpp::nmos_sender_t& config) = 0;

        [[nodiscard]] virtual bisect::maybe_ok remove_resource(const std::string& resource_id,
                                                               const nmos::type& type) = 0;

        [[nodiscard]] virtual bisect::maybe_ok update_clocks(const std::string& clocks) = 0;
    };

    using nmos_uptr = std::unique_ptr<nmos_t>;
} // namespace ossrf
