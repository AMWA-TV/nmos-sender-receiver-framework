#pragma once

#include "bisect/expected.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/json.h"

namespace ossrf
{
    bisect::expected<bisect::nmoscpp::nmos_sender_t> nmos_sender_from_json(const nlohmann::json& device_config);
} // namespace ossrf
