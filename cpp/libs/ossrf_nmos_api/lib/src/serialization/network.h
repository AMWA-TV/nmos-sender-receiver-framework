#pragma once

#include "bisect/expected.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/json/json.h"

namespace ossrf
{
    bisect::expected<bisect::nmoscpp::network_t> network_from_json(const nlohmann::json& config, bool is_receiver);
    bisect::expected<bisect::nmoscpp::network_leg_t> network_leg_from_json(const nlohmann::json& config,
                                                                           bool is_receiver);
} // namespace ossrf
