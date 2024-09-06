#include "serialization/device.h"
#include "serialization/receiver.h"
#include "serialization/sender.h"
#include "bisect/expected/helpers.h"
#include "bisect/json/json.h"
#include <nmos/id.h>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;

using nlohmann::json;

expected<nmos_device_t> ossrf::nmos_device_from_json(const nmos::id node_id, const nlohmann::json& device_config)
{
    nmos_device_t device{};
    device.node_id = node_id;

    BST_CHECK_ASSIGN(device.id, find<std::string>(device_config, "id"));
    BST_CHECK_ASSIGN(device.label, find<std::string>(device_config, "label"));
    BST_CHECK_ASSIGN(device.description, find<std::string>(device_config, "description"));

    return device;
}
