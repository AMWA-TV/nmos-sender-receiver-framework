#pragma once

#include "bisect/expected.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/json.h"

namespace ossrf
{
    bisect::expected<bisect::nmoscpp::nmos_receiver_t> nmos_receiver_from_json(const nlohmann::json& config);
} // namespace ossrf
