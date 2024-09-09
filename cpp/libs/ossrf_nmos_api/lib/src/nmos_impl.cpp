// Copyright (C) 2024 Advanced Media Workflow Association
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ossrf/nmos/api/nmos_impl.h"
#include "bisect/nmoscpp/nmos_controller.h"
#include "bisect/nmoscpp/logger.h"
#include "utils.h"
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>

using namespace bisect;
using namespace bisect::nmoscpp;
using namespace ossrf;
using json = nlohmann::json;

struct nmos_impl::impl
{
    nmos::id node_id_;
    nmos_controller_uptr controller_;
    logger_t log_;
};

nmos_uptr nmos_impl::create(const std::string& node_id)
{
    auto i = std::make_unique<impl>(utility::s2us(node_id));

    return nmos_uptr(new nmos_impl{std::move(i)});
}

nmos_impl::nmos_impl(std::unique_ptr<impl>&& i) noexcept : impl_(std::move(i)) {};

nmos_impl::~nmos_impl()
{
    impl_->controller_->close();
}

maybe_ok nmos_impl::add_node(const std::string& node_configuration, nmos_event_handler_t* nmos_event_handler) noexcept
{
    web::json::value config = web::json::value::parse(node_configuration);

    nmos_controller_t::options_t options{};

    if(config.has_field(U("interfaces")))
    {
        options.interfaces = config.at(U("interfaces"));
    }

    if(config.has_field(U("clocks")))
    {
        options.clocks = config.at(U("clocks"));
    }

    impl_->controller_ = nmos_controller_uptr(new nmos_controller_t(impl_->log_, config, nmos_event_handler));
    auto node          = impl_->controller_->make_node(impl_->node_id_, options);
    BST_CHECK(impl_->controller_->insert_resource(std::move(node)));
    impl_->controller_->open();

    return {};
}

maybe_ok nmos_impl::add_device(const nmos_device_t& config) noexcept
{
    std::vector<std::string> receiver_ids;
    std::vector<std::string> sender_ids;

    auto device = impl_->controller_->make_device(config, receiver_ids, sender_ids);
    return impl_->controller_->insert_resource(std::move(device));
}

maybe_ok nmos_impl::add_receiver(const std::string& device_id, const nmos_receiver_t& config) noexcept
{
    auto receiver = impl_->controller_->make_receiver(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->insert_resource(std::move(receiver)));

    auto connection_receiver = impl_->controller_->make_connection_receiver(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->insert_connection_resource(std::move(connection_receiver)));

    return {};
}

maybe_ok nmos_impl::add_sender(const std::string& device_id, const nmos_sender_t& config) noexcept
{

    BST_ASSIGN_MUT(source, impl_->controller_->make_source(utility::s2us(device_id), config));
    BST_CHECK(impl_->controller_->insert_resource(std::move(source)));

    if(std::holds_alternative<video_sender_info_t>(config.media))
    {
        const auto& video = std::get<video_sender_info_t>(config.media);
        BST_ASSIGN_MUT(flow,
                       impl_->controller_->make_video_flow(utility::s2us(device_id), utility::s2us(config.source.id),
                                                           config.flow, config.media_type, video));
        BST_CHECK(impl_->controller_->insert_resource(std::move(flow)));
    }
    else if(std::holds_alternative<audio_sender_info_t>(config.media))
    {
        const auto& audio = std::get<audio_sender_info_t>(config.media);
        BST_ASSIGN_MUT(flow, impl_->controller_->make_audio_flow(utility::s2us(device_id),
                                                                 utility::s2us(config.source.id), config.flow, audio));
        BST_CHECK(impl_->controller_->insert_resource(std::move(flow)));
    }

    auto sender = impl_->controller_->make_sender(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->insert_resource(std::move(sender)));

    auto connection_sender = impl_->controller_->make_connection_sender(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->insert_connection_resource(std::move(connection_sender)));

    return {};
}

maybe_ok nmos_impl::modify_device(const nmos_device_t& config) noexcept
{
    std::vector<std::string> receiver_ids;
    std::vector<std::string> sender_ids;

    auto device_resource = impl_->controller_->make_device(config, receiver_ids, sender_ids);
    return impl_->controller_->modify_resource(utility::s2us(config.id),
                                               [&](nmos::resource& resource) { resource = device_resource; });
}

maybe_ok nmos_impl::modify_device_sub_resources(const std::string& device_id, const std::vector<std::string>& receivers,
                                                const std::vector<std::string>& senders) noexcept
{
    return impl_->controller_->modify_resource(device_id, [&](nmos::resource& resource) {
        resource.data[nmos::fields::receivers] = web::json::value_from_elements(receivers);
        resource.data[nmos::fields::senders]   = web::json::value_from_elements(senders);
    });
}

maybe_ok nmos_impl::modify_receiver(const std::string& device_id, const nmos_receiver_t& config) noexcept
{
    auto receiver = impl_->controller_->make_receiver(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->modify_resource(utility::s2us(config.id),
                                                  [&](nmos::resource& resource) { resource.data = receiver.data; }));

    BST_CHECK(impl_->controller_->modify_connection_receiver(config));

    return {};
}

maybe_ok nmos_impl::modify_sender(const std::string& device_id, const nmos_sender_t& config) noexcept
{
    BST_ASSIGN_MUT(new_source, impl_->controller_->make_source(utility::s2us(device_id), config));
    BST_CHECK(impl_->controller_->modify_resource(new_source.id,
                                                  [&](nmos::resource& resource) { resource.data = new_source.data; }));

    if(std::holds_alternative<video_sender_info_t>(config.media))
    {
        const auto& video = std::get<video_sender_info_t>(config.media);
        BST_ASSIGN_MUT(new_flow,
                       impl_->controller_->make_video_flow(utility::s2us(device_id), utility::s2us(config.source.id),
                                                           config.flow, config.media_type, video));
        BST_CHECK(impl_->controller_->modify_resource(
            new_flow.id, [&](nmos::resource& resource) { resource.data = new_flow.data; }));
    }
    else if(std::holds_alternative<audio_sender_info_t>(config.media))
    {
        const auto& audio = std::get<audio_sender_info_t>(config.media);
        BST_ASSIGN_MUT(new_flow, impl_->controller_->make_audio_flow(
                                     utility::s2us(device_id), utility::s2us(config.source.id), config.flow, audio));
        BST_CHECK(impl_->controller_->modify_resource(
            new_flow.id, [&](nmos::resource& resource) { resource.data = new_flow.data; }));
    }

    auto new_sender = impl_->controller_->make_sender(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->modify_resource(utility::s2us(config.id),
                                                  [&](nmos::resource& resource) { resource.data = new_sender.data; }));

    auto new_connection_sender = impl_->controller_->make_connection_sender(utility::s2us(device_id), config);
    BST_CHECK(impl_->controller_->modify_connection_resource(
        utility::s2us(config.id), [&](nmos::resource& resource) { resource.data = new_connection_sender.data; }));

    return {};
}

maybe_ok nmos_impl::remove_resource(const std::string& resource_id, const nmos::type& type) noexcept
{
    BST_ENFORCE(impl_->controller_->has_resource(utility::s2us(resource_id), type),
                "NMOS resource with ID  {} was not found.", utility::us2s(resource_id));

    if(type == nmos::types::device)
    {
        return impl_->controller_->erase_device(utility::s2us(resource_id));
    }

    BST_CHECK(impl_->controller_->erase_resource(utility::s2us(resource_id)));

    if(type == nmos::types::receiver || type == nmos::types::sender)
    {
        BST_CHECK(impl_->controller_->erase_connection_resource(utility::s2us(resource_id)));
    }
    return {};
}

maybe_ok nmos_impl::update_clocks(const std::string& clocks) noexcept
{
    auto j_clocks = json::parse(clocks);

    impl_->controller_->modify_resource(impl_->node_id_, [&](nmos::resource& resource) {
        resource.data[utility::conversions::to_string_t("clocks")] = web::json::value::parse(j_clocks.dump());
    });

    BST_CHECK(impl_->controller_->call_senders_with(impl_->node_id_, [&](nmos::resource& resource) -> maybe_ok {
        return impl_->controller_->update_transport_file(resource.id);
    }));

    return {};
}
