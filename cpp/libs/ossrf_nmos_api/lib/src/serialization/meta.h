#pragma once

#include "bisect/expected/helpers.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/json.h"

namespace ossrf
{
    bisect::maybe_ok nmos_meta_from_json(const nlohmann::json& config, bisect::nmoscpp::meta_info_t& info);
}
