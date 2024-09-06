#pragma once

#include "bisect/expected.h"
#include "bisect/json.h"
#include <nmos/rational.h>

namespace ossrf
{
    bisect::expected<nmos::rational> framerate_from_json(const nlohmann::json& config);
}
