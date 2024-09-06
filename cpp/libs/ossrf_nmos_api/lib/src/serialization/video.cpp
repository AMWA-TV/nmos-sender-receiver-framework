#include "serialization/video.h"
#include "bisect/expected/macros.h"

using namespace bisect;

expected<nmos::rational> ossrf::framerate_from_json(const nlohmann::json& config)
{
    BST_ASSIGN(numerator, find<int64_t>(config, "num"));
    BST_ASSIGN(denominator, find<int64_t>(config, "den"));
    return nmos::rational{numerator, denominator};
}
