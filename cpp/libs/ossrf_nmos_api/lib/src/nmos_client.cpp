#include "ossrf/nmos/api/nmos_client.h"
#include "ossrf/nmos/api/nmos_impl.h"
#include "context/context.h"
#include "context/nmos_event_handler.h"
#include "serialization/device.h"
#include "serialization/receiver.h"
#include "serialization/sender.h"
#include "resources/nmos_resource_receiver.h"
#include "resources/nmos_resource_sender.h"
#include "bisect/expected/macros.h"
#include <nlohmann/json.hpp>

using namespace bisect;
using namespace ossrf;
using json = nlohmann::json;

struct nmos_client_t::impl
{
    std::string node_id_;
    nmos_context_ptr context_;
    nmos_event_handler event_handler_;
};

expected<nmos_client_uptr> nmos_client_t::create(const std::string& node_id,
                                                 const std::string& node_configuration) noexcept
{
    auto context       = std::make_shared<nmos_context>(node_id);
    auto event_handler = nmos_event_handler{context};

    auto i = std::make_unique<impl>(node_id, std::move(context), std::move(event_handler));

    BST_CHECK(i->context_->nmos().add_node(node_configuration, &i->event_handler_));

    return nmos_client_uptr(new nmos_client_t{std::move(i)});
}

nmos_client_t::nmos_client_t(std::unique_ptr<impl>&& i) noexcept : impl_(std::move(i)){};

nmos_client_t::~nmos_client_t()
{
    // TODO: Check if deleting the node, delete every other resource
    impl_->context_->resources().erase(impl_->node_id_);
};

maybe_ok nmos_client_t::add_device(const std::string& config) noexcept
{
    BST_ASSIGN_MUT(device_config, nmos_device_from_json(impl_->node_id_, json::parse(config)));

    BST_CHECK(impl_->context_->nmos().add_device(device_config));
    return {};
}

maybe_ok nmos_client_t::add_receiver(const std::string& device_id, const std::string& config,
                                     bisect::nmoscpp::receiver_activation_callback_t callback) noexcept
{
    BST_ASSIGN_MUT(receiver_config, nmos_receiver_from_json(json::parse(config)));

    receiver_config.master_enable = true;

    BST_CHECK(impl_->context_->nmos().add_receiver(device_id, receiver_config));

    auto r = std::make_shared<nmos_resource_receiver_t>(device_id, receiver_config, callback);
    impl_->context_->resources().insert(receiver_config.id, std::move(r));

    return {};
}

maybe_ok nmos_client_t::add_sender(const std::string& device_id, const std::string& config,
                                   bisect::nmoscpp::sender_activation_callback_t callback) noexcept
{
    BST_ASSIGN_MUT(sender_config, nmos_sender_from_json(json::parse(config)));

    sender_config.master_enable = true;

    BST_CHECK(impl_->context_->nmos().add_sender(device_id, sender_config));

    auto r = std::make_shared<nmos_resource_sender_t>(device_id, sender_config, callback);
    impl_->context_->resources().insert(sender_config.id, std::move(r));

    return {};
}

maybe_ok nmos_client_t::remove_resource(const std::string& id, const nmos::type& type) noexcept
{
    BST_CHECK(impl_->context_->nmos().remove_resource(id, type));
    impl_->context_->resources().erase(id);

    return {};
}
