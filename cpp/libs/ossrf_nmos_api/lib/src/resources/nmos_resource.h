#pragma once

#include "bisect/expected.h"
#include <nlohmann/json_fwd.hpp>
#include <nmos/id.h>
#include <string>
#include <memory>

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
    };

    using nmos_resource_ptr  = std::shared_ptr<nmos_resource_t>;
    using nmos_resource_uptr = std::unique_ptr<nmos_resource_t>;
} // namespace ossrf
