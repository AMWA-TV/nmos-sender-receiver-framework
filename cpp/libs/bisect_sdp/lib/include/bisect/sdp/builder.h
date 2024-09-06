#pragma once

#include "bisect/sdp/settings.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"

#include <array>
#include <string>
#include <variant>

namespace bisect::sdp
{
    expected<std::string> build_sdp(const sdp_settings_t& settings);
} // namespace bisect::sdp
