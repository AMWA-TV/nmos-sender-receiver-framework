#pragma once
#include "bisect/expected.h"
#include <nmos/resources.h>

namespace bisect::nmoscpp
{
    class nmos_event_handler_t
    {
      public:
        virtual ~nmos_event_handler_t() = default;

        [[nodiscard]] virtual bisect::expected<std::string>
        handle_active_state_changed(const nmos::resource& resource, const nmos::resource& connection_resource,
                                    const std::string& transport_params) = 0;

        [[nodiscard]] virtual maybe_ok handle_patch_request(const nmos::resource& resource,
                                                            const nmos::resource& connection_resource,
                                                            const std::string& endpoint_staged) = 0;
    };

} // namespace bisect::nmoscpp
