#pragma once

#include "bisect/sdp/settings.h"
#include "bisect/expected.h"

#include <string>

namespace bisect::sdp
{
    expected<sdp_settings_t> parse_sdp(const std::string& sdp);
} // namespace bisect::sdp