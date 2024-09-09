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

#include "bisect/nmoscpp/nmos_controller.h"
#include "bisect/nmoscpp/detail/expected.h"
#include "bisect/nmoscpp/detail/internal.h"
#include "utils.h"
#include <nmos/did_sdid.h>
#include <nmos/settings.h>
#include <nmos/resources.h>
#include <nmos/model.h>
#include <nmos/colorspace.h>
#include <nmos/transfer_characteristic.h>
#include <nmos/components.h>
#include <nmos/node_interfaces.h>
#include <nmos/node_resource.h>
#include <nmos/clock_name.h>
#include <nmos/activation_mode.h>
#include <nmos/sdp_utils.h>
#include <nmos/format.h>
#include <nmos/media_type.h>
#include <nmos/id.h>
#include <cpprest/host_utils.h>

using namespace bisect::core::detail;
using namespace bisect::nmoscpp;
using namespace bisect;

namespace
{
    constexpr auto delay_millis{0};

    std::vector<utility::string_t> get_interface_names_from_network(const network_t& net)
    {
        std::vector<utility::string_t> names;

        if(net.primary.interface_name.has_value())
        {
            names.push_back(utility::s2us(net.primary.interface_name.value()));
        }

        if(net.secondary.has_value())
        {
            const auto& sec = net.secondary.value();
            if(sec.interface_name.has_value())
            {
                names.push_back(utility::s2us(sec.interface_name.value()));
            }
        }

        return names;
    }

    expected<nmos::resource> do_make_source(const nmos::id& device_id, const nmos_sender_t& sender,
                                            const nmos::settings& settings)
    {
        if(sender.format == nmos::formats::video)
        {
            const auto& video     = std::get<video_sender_info_t>(sender.media);
            const auto grain_rate = video.exact_framerate;
            return nmos::make_video_source(utility::s2us(sender.source.id), device_id, nmos::clock_names::clk0,
                                           grain_rate, settings);
        }
        else if(sender.format == nmos::formats::audio)
        {
            const auto& audio        = std::get<audio_sender_info_t>(sender.media);
            const auto grain_rate    = nmos::rational(audio.sampling_rate, 1);
            const auto channel_count = audio.number_of_channels;

            const auto channels = boost::copy_range<std::vector<nmos::channel>>(
                boost::irange(0, channel_count) | boost::adaptors::transformed([&](const int& index) {
                    return bisect::channels_repeat[static_cast<unsigned long>(
                        index % static_cast<int>(bisect::channels_repeat.size()))];
                }));

            return nmos::make_audio_source(utility::s2us(sender.source.id), device_id, nmos::clock_names::clk0,
                                           grain_rate, channels, settings);
        }

        BST_FAIL("invalid format");
    }

    web::json::value get_default_interfaces(const nmos::settings& settings)
    {
        const auto host_interfaces = nmos::get_host_interfaces(settings);
        const auto interfaces      = nmos::experimental::node_interfaces(host_interfaces);
        return nmos::make_node_interfaces(interfaces);
    }

    template <typename T> void set_or_previous_or_auto(web::json::value& param, const std::optional<T>& v)
    {
        if(v.has_value())
        {
            param = v.value();
        }
        else if(param.is_null())
        {
            param = web::json::value::string(utility::s2us("auto"));
        }
    }

    template <typename T, typename F> auto map(const std::optional<T>& v, F f) -> std::optional<decltype(f(v.value()))>
    {
        if(v.has_value())
        {
            return std::make_optional(f(v.value()));
        }

        return std::nullopt;
    }

    void set_transport_params(const network_leg_t& leg, web::json::value& params, bool is_receiver)
    {
        // Common: `source_ip`, `destination_port`, `rtp_enabled`
        params[nmos::fields::rtp_enabled] = web::json::value::boolean(leg.rtp_enabled);
        set_or_previous_or_auto(params[nmos::fields::source_ip],
                                map(leg.source_ip, [](auto v) { return web::json::value::string(utility::s2us(v)); }));
        set_or_previous_or_auto(params[nmos::fields::destination_port],
                                map(leg.destination_port, [](auto v) { return web::json::value::number(v); }));

        if(is_receiver)
        {
            // Receiver: `interface_ip`, `multicast_ip`
            set_or_previous_or_auto(params[nmos::fields::interface_ip], map(leg.interface_ip, [](auto v) {
                                        return web::json::value::string(utility::s2us(v));
                                    }));
            set_or_previous_or_auto(params[nmos::fields::multicast_ip], map(leg.destination_ip, [](auto v) {
                                        return web::json::value::string(utility::s2us(v));
                                    }));
        }
        else
        {
            // Sender: `destination_ip`, `source_port`
            set_or_previous_or_auto(params[nmos::fields::destination_ip], map(leg.destination_ip, [](auto v) {
                                        return web::json::value::string(utility::s2us(v));
                                    }));
            set_or_previous_or_auto(params[nmos::fields::source_port],
                                    map(leg.source_port, [](auto v) { return web::json::value::number(v); }));
        }
    }

    void update_connection_receiver_staged_params(const nmos_receiver_t& config, web::json::value& staged)
    {
        if(config.sender_id.has_value())
        {
            staged[nmos::fields::sender_id] = web::json::value::string(utility::s2us(config.sender_id.value()));
        }

        if(config.sdp_data.has_value())
        {
            auto tf                              = web::json::value::object();
            tf[nmos::fields::transportfile_type] = web::json::value::string(utility::s2us("application/sdp"));
            tf[nmos::fields::transportfile_data] = web::json::value::string(utility::s2us(config.sdp_data.value()));
            staged[nmos::fields::transport_file] = tf;
        }

        staged[nmos::fields::master_enable] = web::json::value::boolean(config.master_enable);
        set_transport_params(config.network.primary, staged[nmos::fields::transport_params][0], true);
        if(config.network.secondary.has_value())
        {
            set_transport_params(config.network.secondary.value(), staged[nmos::fields::transport_params][1], true);
        }

        staged[nmos::fields::activation] =
            web::json::value_of({{nmos::fields::mode, nmos::activation_modes::activate_scheduled_relative.name},
                                 {nmos::fields::requested_time, _XPLATSTR("0:0")},
                                 {nmos::fields::activation_time, nmos::make_version()}});
    }
} // namespace

nmos_controller_t::nmos_controller_t(logger_t& logger, web::json::value configuration,
                                     nmos_event_handler_t* event_handler)
    : base_controller_(logger.gate(), configuration, event_handler),
      server_(nmos::experimental::make_node_server(base_controller_.node_model_, base_controller_.node_implementation_,
                                                   logger.model(), logger.gate()))
{
    resolve_auto_ = make_node_implementation_auto_resolver(base_controller_.node_model_.settings, event_handler);

    insert_resource_after_ = [this](unsigned int milliseconds, nmos::resources& resources,
                                    nmos::resource&& resource) -> maybe_ok {
        auto lock = base_controller_.node_model_.write_lock();
        if(nmos::details::wait_for(base_controller_.node_model_.shutdown_condition, lock,
                                   std::chrono::milliseconds(milliseconds),
                                   [&] { return base_controller_.node_model_.shutdown; }))
        {
            BST_FAIL("Could not lock node model in order to write the new resources in it");
        }

        const std::pair<nmos::id, nmos::type> id_type{resource.id, resource.type};

        const auto result = nmos::insert_resource(resources, std::move(resource)).second;
        if(!result)
        {
            slog::log<slog::severities::severe>(base_controller_.gate_, SLOG_FLF) << "Model update error: " << id_type;

            slog::log<slog::severities::too_much_info>(base_controller_.gate_, SLOG_FLF)
                << "Notifying node behaviour thread"; // and anyone else who cares...
            base_controller_.node_model_.notify();

            BST_FAIL("Error updating model with a {} resource with id {}.", utility::us2s(id_type.second.name),
                     utility::us2s(id_type.first));
        }

        slog::log<slog::severities::info>(base_controller_.gate_, SLOG_FLF) << "Updated model with " << id_type;
        slog::log<slog::severities::too_much_info>(base_controller_.gate_, SLOG_FLF)
            << "Notifying node behaviour thread"; // and anyone else who cares...
        base_controller_.node_model_.notify();
        return {};
    };

    modify_resource_after_ = [this](unsigned int milliseconds, nmos::resources& resources, const nmos::id& id,
                                    std::function<void(nmos::resource&)> modifier) -> maybe_ok {
        auto lock = base_controller_.node_model_.write_lock();
        if(nmos::details::wait_for(base_controller_.node_model_.shutdown_condition, lock,
                                   std::chrono::milliseconds(milliseconds),
                                   [&] { return base_controller_.node_model_.shutdown; }))
        {
            BST_FAIL("Could not lock node model in order to write the new resources in it");
        }

        auto result = nmos::modify_resource(resources, id, [modifier](nmos::resource& resource) {
            modifier(resource);
            resource.data[nmos::fields::version] = web::json::value(nmos::make_version());
        });
        if(!result)
        {
            slog::log<slog::severities::severe>(base_controller_.gate_, SLOG_FLF) << "Model update error: " << id;

            slog::log<slog::severities::too_much_info>(base_controller_.gate_, SLOG_FLF)
                << "Notifying node behaviour thread"; // and anyone else who cares...
            base_controller_.node_model_.notify();

            BST_FAIL("Error updating resource with id {}.", utility::us2s(id));
        }

        slog::log<slog::severities::info>(base_controller_.gate_, SLOG_FLF) << "modified resource with id:" << id;
        slog::log<slog::severities::too_much_info>(base_controller_.gate_, SLOG_FLF)
            << "Notifying node behaviour thread"; // and anyone else who cares...
        base_controller_.node_model_.notify();
        return {};
    };

    erase_resource_after_ = [this](unsigned int milliseconds, nmos::resources& resources,
                                   const nmos::id& resource_id) -> maybe_ok {
        auto lock = base_controller_.node_model_.write_lock();
        if(nmos::details::wait_for(base_controller_.node_model_.shutdown_condition, lock,
                                   std::chrono::milliseconds(milliseconds),
                                   [&] { return base_controller_.node_model_.shutdown; }))
        {
            BST_FAIL("Could not lock node model in order to remove resources from it");
        }

        nmos::resources::size_type resources_deleted   = nmos::erase_resource(resources, resource_id, false);
        nmos::resources::size_type resources_forgotten = nmos::forget_erased_resources(resources);
        if(resources_deleted <= 0)
        {
            slog::log<slog::severities::severe>(base_controller_.gate_, SLOG_FLF)
                << "Error deleting from model resource with id: " << resource_id;
            slog::log<slog::severities::too_much_info>(base_controller_.gate_, SLOG_FLF)
                << "Notifying node behaviour thread"; // and anyone else who cares...
            base_controller_.node_model_.notify();

            BST_FAIL("Error deleting from model resource with id: {}", utility::us2s(resource_id));
        }
        slog::log<slog::severities::info>(base_controller_.gate_, SLOG_FLF)
            << "Deleted " << resources_deleted << " resources";
        slog::log<slog::severities::info>(base_controller_.gate_, SLOG_FLF)
            << "Forgot " << resources_forgotten << " resources";

        slog::log<slog::severities::info>(base_controller_.gate_, SLOG_FLF)
            << "Deleted from model resource with id " << resource_id;
        slog::log<slog::severities::too_much_info>(base_controller_.gate_, SLOG_FLF)
            << "Notifying node behaviour thread"; // and anyone else who cares...
        base_controller_.node_model_.notify();
        return {};
    };

    if(!nmos::experimental::fields::http_trace(base_controller_.node_model_.settings))
    {
        // Disable TRACE method
        for(auto& http_listener : server_.http_listeners)
        {
            http_listener.support(web::http::methods::TRCE, [](web::http::http_request req) {
                req.reply(web::http::status_codes::MethodNotAllowed);
            });
        }
    }
}

void nmos_controller_t::open()
{
    server_.open().wait();
}

void nmos_controller_t::close()
{
    server_.close().wait();
}

template <typename F> web::json::value get_option_or_default(const nmos_controller_t::opt_json& opt, F def)
{
    if(opt.has_value())
    {
        return opt.value();
    }

    return def();
}

nmos::resource nmos_controller_t::make_node(const nmos::id& node_id, options_t options)
{

    const auto clocks = get_option_or_default(
        options.clocks, []() { return web::json::value_of({nmos::make_internal_clock(nmos::clock_names::clk0)}); });
    const auto interfaces =
        get_option_or_default(options.interfaces, [settings = &base_controller_.node_model_.settings]() {
            return get_default_interfaces(*settings);
        });

    return nmos::make_node(node_id, clocks, interfaces, base_controller_.node_model_.settings);
}

nmos::resource nmos_controller_t::make_device(const nmos_device_t& device_config,
                                              const std::vector<nmos::id>& receivers_ids,
                                              const std::vector<nmos::id>& senders_ids)
{
    auto device = nmos::make_device(utility::s2us(device_config.id), utility::s2us(device_config.node_id), senders_ids,
                                    receivers_ids, base_controller_.node_model_.settings);
    bisect::set_label(device, device_config.label);
    bisect::set_description(device, device_config.description);

    return device;
}

nmos::resource nmos_controller_t::make_receiver(const nmos::id& device_id, const nmos_receiver_t& config)
{
    auto receiver = nmos::make_receiver(utility::s2us(config.id), device_id, nmos::transports::rtp_mcast,
                                        get_interface_names_from_network(config.network), config.format,
                                        config.media_types, base_controller_.node_model_.settings);

    bisect::set_label(receiver, config.label);
    bisect::set_description(receiver, config.description);

    receiver.data[nmos::fields::version] = receiver.data[nmos::fields::caps][nmos::fields::version] =
        web::json::value(nmos::make_version());
    return receiver;
}

nmos::resource nmos_controller_t::make_connection_receiver(const nmos::id& device_id, const nmos_receiver_t& config)
{
    const auto is_st2022_7 = config.network.secondary.has_value();

    auto connection_receiver = nmos::make_connection_rtp_receiver(utility::s2us(config.id), is_st2022_7);

    if(config.network.primary.interface_ip.has_value())
    {
        auto v = web::json::value::array();
        web::json::push_back(v, utility::s2us(config.network.primary.interface_ip.value()));
        connection_receiver.data[nmos::fields::endpoint_constraints][0][nmos::fields::interface_ip] =
            web::json::value_of({{nmos::fields::constraint_enum, v}});
    }

    if(config.network.secondary.has_value() && config.network.secondary.value().interface_ip.has_value())
    {
        auto v = web::json::value::array();
        web::json::push_back(v, utility::s2us(config.network.secondary.value().interface_ip.value()));
        connection_receiver.data[nmos::fields::endpoint_constraints][1][nmos::fields::interface_ip] =
            web::json::value_of({{nmos::fields::constraint_enum, v}});
    }

    auto& staged = connection_receiver.data[nmos::fields::endpoint_staged];
    update_connection_receiver_staged_params(config, staged);
    return connection_receiver;
}

expected<nmos::resource> nmos_controller_t::make_source(const nmos::id& device_id, const nmos_sender_t& config)
{
    BST_ASSIGN_MUT(source, do_make_source(device_id, config, base_controller_.node_model_.settings));
    bisect::set_label(source, config.source.label);
    bisect::set_description(source, config.source.description);

    return source;
}

expected<nmos::resource> nmos_controller_t::make_audio_flow(const nmos::id& device_id, const nmos::id& source_id,
                                                            const flow_t& flow_config, const audio_sender_info_t& media)
{
    auto flow = nmos::make_raw_audio_flow(utility::s2us(flow_config.id), source_id, device_id, media.sampling_rate,
                                          media.bits_per_sample, base_controller_.node_model_.settings);
    flow.data[nmos::fields::grain_rate] = nmos::make_rational(media.sampling_rate);
    bisect::set_label(flow, flow_config.label);
    bisect::set_description(flow, flow_config.description);

    return flow;
}

expected<nmos::resource> nmos_controller_t::make_video_flow(const nmos::id& device_id, const nmos::id& source_id,
                                                            const flow_t& flow_config, std::string media_type,
                                                            const video_sender_info_t& media)
{
    const auto frame_width  = static_cast<unsigned int>(media.width);
    const auto frame_height = static_cast<unsigned int>(media.height);

    // TODO: receive these parameters
    const auto bit_depth               = 10;
    const auto colorspace              = nmos::colorspaces::BT709;
    const auto transfer_characteristic = nmos::transfer_characteristics::SDR;
    const auto color_sampling          = ::sdp::samplings::YCbCr_4_2_2;

    using web::json::value;

    auto flow  = nmos::make_video_flow(utility::s2us(flow_config.id), source_id, device_id, media.exact_framerate,
                                       frame_width, frame_height, media.structure, colorspace, transfer_characteristic,
                                       base_controller_.node_model_.settings);
    auto& data = flow.data;

    data[U("media_type")] = value::string(utility::s2us(media_type));
    data[U("components")] = make_components(color_sampling, frame_width, frame_height, bit_depth);

    if(!flow_config.extra.is_null())
    {
        merge_patch(flow.data, flow_config.extra, true);
    };

    bisect::set_label(flow, flow_config.label);
    bisect::set_description(flow, flow_config.description);

    return flow;
}

nmos::resource nmos_controller_t::make_sender(const nmos::id& device_id, const nmos_sender_t& sender_config)
{
    const auto interface_names = get_interface_names_from_network(sender_config.network);

    const auto manifest_url = nmos::experimental::make_manifest_api_manifest(utility::s2us(sender_config.id),
                                                                             base_controller_.node_model_.settings)
                                  .to_string();

    // TODO: check if interface names are consistent with the ones reported by the node. Warn if otherwise.
    auto sender = nmos::make_sender(utility::s2us(sender_config.id), utility::s2us(sender_config.flow.id),
                                    nmos::transports::rtp_mcast, device_id, manifest_url, interface_names,
                                    base_controller_.node_model_.settings);

    if(!sender_config.extra.is_null())
    {
        merge_patch(sender.data, sender_config.extra, true);
    };

    bisect::set_label(sender, sender_config.label);
    bisect::set_description(sender, sender_config.description);

    return sender;
}

nmos::resource nmos_controller_t::make_connection_sender(const nmos::id& device_id, const nmos_sender_t& sender_config)
{
    const auto is_st2022_7 = sender_config.network.secondary.has_value();

    auto connection_sender = nmos::make_connection_rtp_sender(utility::s2us(sender_config.id), is_st2022_7);
    if(sender_config.network.primary.source_ip.has_value())
    {
        auto v = web::json::value::array();
        web::json::push_back(v, utility::s2us(sender_config.network.primary.source_ip.value()));
        connection_sender.data[nmos::fields::endpoint_constraints][0][nmos::fields::source_ip] =
            web::json::value_of({{nmos::fields::constraint_enum, v}});
    }

    if(sender_config.network.secondary.has_value() && sender_config.network.secondary.value().source_ip.has_value())
    {
        auto v = web::json::value::array();
        web::json::push_back(v, utility::s2us(sender_config.network.secondary.value().source_ip.value()));
        connection_sender.data[nmos::fields::endpoint_constraints][1][nmos::fields::source_ip] =
            web::json::value_of({{nmos::fields::constraint_enum, v}});
    }

    auto& staged = connection_sender.data[nmos::fields::endpoint_staged];

    staged[nmos::fields::master_enable] = web::json::value::boolean(sender_config.master_enable);
    set_transport_params(sender_config.network.primary, staged[nmos::fields::transport_params][0], false);
    if(sender_config.network.secondary.has_value())
    {
        set_transport_params(sender_config.network.secondary.value(), staged[nmos::fields::transport_params][1], false);
    }

    staged[nmos::fields::activation] =
        web::json::value_of({{nmos::fields::mode, nmos::activation_modes::activate_scheduled_relative.name},
                             {nmos::fields::requested_time, _XPLATSTR("0:0")},
                             {nmos::fields::activation_time, nmos::make_version()}});

    return connection_sender;
}

maybe_ok nmos_controller_t::insert_resource(nmos::resource&& resource)
{
    auto id = resource.id;
    BST_CHECK(insert_resource_after_(delay_millis, base_controller_.node_model_.node_resources, std::move(resource)));
    BST_CHECK(find_resource(id));

    return {};
}

maybe_ok nmos_controller_t::insert_connection_resource(nmos::resource&& resource)
{
    auto id = resource.id;
    BST_CHECK(
        insert_resource_after_(delay_millis, base_controller_.node_model_.connection_resources, std::move(resource)));
    BST_CHECK(find_resource(id));

    return {};
}

maybe_ok nmos_controller_t::modify_resource(const nmos::id& resource_id, std::function<void(nmos::resource&)> modifier)
{
    return modify_resource_after_(delay_millis, base_controller_.node_model_.node_resources, resource_id, modifier);
}

maybe_ok nmos_controller_t::modify_connection_resource(const nmos::id& resource_id,
                                                       std::function<void(nmos::resource&)> modifier)
{
    return modify_resource_after_(delay_millis, base_controller_.node_model_.connection_resources, resource_id,
                                  modifier);
}

maybe_ok nmos_controller_t::modify_connection_receiver(const nmos_receiver_t& config)
{
    BST_ASSIGN_MUT(connection_receiver, find_connection_resource(utility::s2us(config.id)))

    connection_receiver.data[nmos::fields::version]         = web::json::value(nmos::make_version());
    connection_receiver.data[nmos::fields::endpoint_staged] = connection_receiver.data[nmos::fields::endpoint_active];
    update_connection_receiver_staged_params(config, connection_receiver.data[nmos::fields::endpoint_staged]);

    BST_CHECK(modify_connection_resource(utility::s2us(config.id),
                                         [&](nmos::resource& resource) { resource.data = connection_receiver.data; }));

    return {};
}

expected<nmos::resource> nmos_controller_t::find_resource(const nmos::id& id)
{
    const auto resource_it = nmos::find_resource(base_controller_.node_model_.node_resources, id);
    BST_ENFORCE(resource_it != base_controller_.node_model_.node_resources.end(),
                "trying to find a non-existing NMOS resource {}", utility::us2s(id));
    return *resource_it;
}

expected<nmos::resource> nmos_controller_t::find_connection_resource(const nmos::id& id)
{
    const auto resource_it = nmos::find_resource(base_controller_.node_model_.connection_resources, id);
    BST_ENFORCE(resource_it != base_controller_.node_model_.connection_resources.end(),
                "trying to find a non-existing NMOS connection resource {}", utility::us2s(id));
    return *resource_it;
}

maybe_ok nmos_controller_t::erase_resource(const nmos::id& resource_id)
{

    BST_CHECK(erase_resource_after_(delay_millis, base_controller_.node_model_.node_resources, resource_id));

    const auto resource = nmos::find_resource(base_controller_.node_model_.node_resources, resource_id);
    BST_ENFORCE(base_controller_.node_model_.node_resources.end() == resource,
                "NMOS Resource with id {} was not deleted", utility::us2s(resource_id));

    return {};
}

maybe_ok nmos_controller_t::erase_connection_resource(const nmos::id& resource_id)
{
    BST_CHECK(erase_resource_after_(delay_millis, base_controller_.node_model_.connection_resources, resource_id));

    const auto resource = nmos::find_resource(base_controller_.node_model_.connection_resources, resource_id);
    BST_ENFORCE(base_controller_.node_model_.connection_resources.end() == resource,
                "NMOS Connection resource with id {} was not deleted", utility::us2s(resource_id));

    return {};
}

maybe_ok nmos_controller_t::erase_device(const nmos::id& device_id)
{
    const auto device =
        nmos::find_resource(base_controller_.node_model_.node_resources, {device_id, nmos::types::device});

    std::vector<maybe_ok> maybe_result_deleting_sub_resources;

    std::transform(
        device->sub_resources.begin(), device->sub_resources.end(),
        std::back_inserter(maybe_result_deleting_sub_resources),
        [this, resources = base_controller_.node_model_.node_resources](const nmos::id& sub_resource_id) -> maybe_ok {
            const auto resource = nmos::find_resource(resources, sub_resource_id);
            if(resources.end() == resource)
            {
                slog::log<slog::severities::severe>(base_controller_.gate_, SLOG_FLF)
                    << "Sub-resource does not exist: " << sub_resource_id;
            }
            if(resource->type == nmos::types::receiver || resource->type == nmos::types::sender)
            {
                return erase_connection_resource(resource->id);
            }
            return {};
        });

    auto maybe_error =
        std::find_if(maybe_result_deleting_sub_resources.begin(), maybe_result_deleting_sub_resources.end(),
                     [&](maybe_ok& result) { return is_error(result); });

    if(maybe_error != std::end(maybe_result_deleting_sub_resources))
    {
        return std::move(*maybe_error);
    }

    BST_CHECK(erase_resource(device_id));

    return {};
}

maybe_ok nmos_controller_t::update_transport_file(const nmos::id& sender_id)
{
    const auto sender_it =
        nmos::find_resource(base_controller_.node_model_.node_resources, {sender_id, nmos::types::sender});
    BST_ENFORCE(sender_it == base_controller_.node_model_.node_resources.end(),
                "trying to update the transport file of a non-existing NMOS sender {}", utility::us2s(sender_id));
    auto& sender = *sender_it;

    modify_connection_resource(sender_id, [this, &sender](nmos::resource& connection_sender) {
        web::json::value endpoint_transportfile;
        auto result = build_transport_file(base_controller_.node_model_.node_resources, base_controller_.event_handler_,
                                           sender, connection_sender, endpoint_transportfile);

        if(is_error(result))
        {
            fmt::print("ERROR updating transport file of sender {}: {}\n", utility::us2s(sender.id),
                       result.error().what());
            return;
        }

        connection_sender.data[nmos::fields::endpoint_transportfile] = endpoint_transportfile;
    });

    return {};
}

bool nmos_controller_t::has_resource(const nmos::id& id, const nmos::type& type)
{
    return nmos::has_resource(base_controller_.node_model_.node_resources, {id, type});
}

std::vector<utility::string_t> nmos_controller_t::get_interfaces_names(const nmos::settings& settings, bool smpte2022_7)
{
    const auto host_interfaces = nmos::get_host_interfaces(settings);
    const auto interfaces      = nmos::experimental::node_interfaces(host_interfaces);

    // prepare interface bindings for all senders and receivers
    const auto& host_address = nmos::fields::host_address(settings);
    // the interface corresponding to the host address is used for the example node's WebSocket senders and
    // receivers
    const auto host_interface_ = bisect::find_interface(host_interfaces, host_address);
    if(host_interfaces.end() == host_interface_)
    {
        slog::log<slog::severities::severe>(base_controller_.gate_, SLOG_FLF)
            << "No network interface corresponding to host_address?";
        return std::vector<utility::string_t>{};
    }

    // hmm, should probably add a custom setting to control the primary and secondary interfaces for the example
    // node's RTP senders and receivers rather than just picking the one(s) corresponding to the first and last of
    // the specified host addresses
    const auto& primary_address     = settings.has_field(nmos::fields::host_addresses)
                                          ? web::json::front(nmos::fields::host_addresses(settings)).as_string()
                                          : host_address;
    const auto& secondary_address   = settings.has_field(nmos::fields::host_addresses)
                                          ? web::json::back(nmos::fields::host_addresses(settings)).as_string()
                                          : host_address;
    const auto primary_interface_   = bisect::find_interface(host_interfaces, primary_address);
    const auto secondary_interface_ = bisect::find_interface(host_interfaces, secondary_address);
    if(host_interfaces.end() == primary_interface_ || host_interfaces.end() == secondary_interface_)
    {
        slog::log<slog::severities::severe>(base_controller_.gate_, SLOG_FLF)
            << "No network interface corresponding to one of the host_addresses?";
        return std::vector<utility::string_t>{};
    }
    const auto& primary_interface   = *primary_interface_;
    const auto& secondary_interface = *secondary_interface_;
    const auto interface_names      = smpte2022_7
                                          ? std::vector<utility::string_t>{primary_interface.name, secondary_interface.name}
                                          : std::vector<utility::string_t>{primary_interface.name};

    return interface_names;
}

maybe_ok nmos_controller_t::call_senders_with(const nmos::id& node_id, std::function<maybe_ok(nmos::resource&)> f)
{
    BST_ASSIGN_MUT(n, find_resource(node_id));

    for(auto& device_id : n.sub_resources)
    {
        BST_ASSIGN_MUT(d, find_resource(device_id));
        for(auto& r_id : d.sub_resources)
        {
            BST_ASSIGN_MUT(r, find_resource(r_id));
            if(r.type == nmos::types::sender)
            {
                BST_CHECK(f(r));
            }
        }
    }
    return {};
}
