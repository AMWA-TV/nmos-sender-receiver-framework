#pragma once

#include "bisect/expected.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/json.h"
#include <nmos/id.h>

namespace ossrf
{
    bisect::expected<bisect::nmoscpp::nmos_device_t> nmos_device_from_json(const nmos::id node_id,
                                                                           const nlohmann::json& device_config);
}
