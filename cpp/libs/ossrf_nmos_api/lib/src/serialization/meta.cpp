#include "serialization/meta.h"
#include "bisect/expected/helpers.h"
#include <nmos/id.h>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;

maybe_ok ossrf::nmos_meta_from_json(const nlohmann::json& config, meta_info_t& info)
{
    BST_CHECK_ASSIGN(info.id, find<std::string>(config, "id"));

    assign_if<std::string>(config, "label", info, &meta_info_t::label);
    assign_if<std::string>(config, "description", info, &meta_info_t::description);

    return {};
}
