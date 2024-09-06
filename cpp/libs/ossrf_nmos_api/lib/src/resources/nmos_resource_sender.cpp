#include "nmos_resource_sender.h"
#include "bisect/sdp/media_types.h"
#include "../serialization/sender.h"
#include <nmos/id.h>

using namespace ossrf;
using namespace bisect;
using namespace bisect::nmoscpp;
using json = nlohmann::json;

nmos_resource_sender_t::nmos_resource_sender_t(const std::string& device_id, const nmos_sender_t& config,
                                               sender_activation_callback_t callback)
    : config_(config), activation_callback_(callback), device_id_(device_id)
{
}

const std::string& nmos_resource_sender_t::get_device_id() const
{
    return device_id_;
}

const std::string& nmos_resource_sender_t::get_id() const
{
    return config_.id;
}

maybe_ok nmos_resource_sender_t::handle_activation(bool master_enable, json& transport_params)
{

    // TODO: Resolve auto params
    activation_callback_(master_enable, transport_params);

    master_enable_ = master_enable;

    return {};
}

maybe_ok nmos_resource_sender_t::handle_patch(bool master_enable, const json& configuration)
{
    return {};
}
