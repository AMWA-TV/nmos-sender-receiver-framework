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

#include "utils.h"
#include "bisect/nmoscpp/detail/expected.h"
#include <boost/range/algorithm/find_if.hpp>
#include <nmos/group_hint.h>
#include <nmos/rational.h>
#include <nmos/format.h>
#include <nmos/sdp_utils.h>
#include <sdp/sdp.h>
#include <variant>

using namespace web::json;
using namespace bisect::core::detail;
using namespace bisect::nmoscpp;
namespace conan_sdp = sdp;

// Example Connection API activation callback to resolve "auto" values when /staged is transitioned to /active
nmos::connection_resource_auto_resolver
bisect::nmoscpp::make_node_implementation_auto_resolver(const nmos::settings& settings,
                                                        nmos_event_handler_t* event_handler)
{
    // although which properties may need to be defaulted depends on the resource type,
    // the default value will almost always be different for each resource
    return [&settings, event_handler](const nmos::resource& resource, const nmos::resource& connection_resource,
                                      value& transport_params) {
        fmt::print("auto_resolver - type: {}, transport: {}, initial: {}\n", utility::us2s(resource.type.name),
                   utility::us2s(resource.data.at(U("transport")).as_string()),
                   utility::us2s(transport_params.serialize()));

        auto result = event_handler->handle_active_state_changed(resource, connection_resource,
                                                                 utility::us2s(transport_params.serialize()));

        if(is_error(result))
        {
            throw web::json::json_exception(result.error().what());
        }

#if 0
        // this code relies on the specific constraints added by node_implementation_thread
        const std::pair<nmos::id, nmos::type> id_type{connection_resource.id, connection_resource.type};
        const auto& constraints = nmos::fields::endpoint_constraints(connection_resource.data);
        // "In some cases the behaviour is more complex, and may be determined by the vendor."
        // See
        // https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/docs/2.2.%20APIs%20-%20Server%20Side%20Implementation.md#use-of-auto
        if(resource.type == nmos::types::sender &&
           resource.data.at(U("transport")).as_string().starts_with("urn:x-nmos:transport:rtp"))
        {
            fmt::print("starting sender auto resolver with: {}", transport_params.serialize());
            const bool smpte2022_7 = 1 < transport_params.size();
            nmos::details::resolve_auto(transport_params[0], nmos::fields::source_ip, [&] {
                return web::json::front(nmos::fields::constraint_enum(constraints.at(0).at(nmos::fields::source_ip)));
            });
            if(smpte2022_7)
                nmos::details::resolve_auto(transport_params[1], nmos::fields::source_ip, [&] {
                    return web::json::back(
                        nmos::fields::constraint_enum(constraints.at(1).at(nmos::fields::source_ip)));
                });
            nmos::details::resolve_auto(transport_params[0], nmos::fields::destination_ip, [&] {
                return value::string(bisect::make_source_specific_multicast_address_v4(id_type.first, 0));
            });
            if(smpte2022_7)
                nmos::details::resolve_auto(transport_params[1], nmos::fields::destination_ip, [&] {
                    return value::string(bisect::make_source_specific_multicast_address_v4(id_type.first, 1));
                });
            // lastly, apply the specification defaults for any properties not handled above
            nmos::resolve_rtp_auto(id_type.second, transport_params);
            fmt::print("sender auto resolver completed with: {}", transport_params.serialize());
        }
        else if(resource.type == nmos::types::receiver &&
                resource.data.at(U("transport")).as_string().starts_with("urn:x-nmos:transport:rtp"))
        {
            const bool smpte2022_7 = 1 < transport_params.size();
            nmos::details::resolve_auto(transport_params[0], nmos::fields::interface_ip, [&] {
                return web::json::front(
                    nmos::fields::constraint_enum(constraints.at(0).at(nmos::fields::interface_ip)));
            });
            if(smpte2022_7)
                nmos::details::resolve_auto(transport_params[1], nmos::fields::interface_ip, [&] {
                    return web::json::back(
                        nmos::fields::constraint_enum(constraints.at(1).at(nmos::fields::interface_ip)));
                });
            // lastly, apply the specification defaults for any properties not handled above
            nmos::resolve_rtp_auto(id_type.second, transport_params);
        }
        else if(resource.type == nmos::types::sender &&
                resource.data.at(U("transport")).as_string() == "urn:x-nmos:transport:websocket")
        {
            const auto device = get_super_resource(resource);
            if(device.second == nmos::types::device)
            {
                const auto ws_sender_uri = nmos::make_events_ws_api_connection_uri(device.first, settings);

                nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_uri,
                                            [&] { return value::string(ws_sender_uri.to_string()); });
                nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization,
                                            [&] { return value::boolean(false); });
            }
        }
        else if(resource.type == nmos::types::receiver &&
                resource.data.at(U("transport")).as_string() == "urn:x-nmos:transport:websocket")
        {
            nmos::details::resolve_auto(transport_params[0], nmos::fields::connection_authorization,
                                        [&] { return value::boolean(false); });
        }
#endif

        fmt::print("auto_resolver - final: {}\n", utility::us2s(transport_params.serialize()));
    };
}

// find interface with the specified address
std::vector<web::hosts::experimental::host_interface>::const_iterator
bisect::find_interface(const std::vector<web::hosts::experimental::host_interface>& interfaces,
                       const utility::string_t& address)
{
    return boost::range::find_if(interfaces, [&](const web::hosts::experimental::host_interface& interface) {
        return interface.addresses.end() != boost::range::find(interface.addresses, address);
    });
}

// add a helpful suffix to the label of a sub-resource for the example node
void bisect::set_label_description(nmos::resource& resource, const bisect::port& port)
{
    // TODO VERIFY IF I CAN REMOVE INDEX
    auto label = nmos::fields::label(resource.data);
    if(!label.empty()) label += U('/');
    label += resource.type.name + U('/') + port.name;
    resource.data[nmos::fields::label] = value::string(label);

    auto description = nmos::fields::description(resource.data);
    if(!description.empty()) description += U('/');
    description += resource.type.name + U('/') + port.name;
    resource.data[nmos::fields::description] = value::string(description);
}

void bisect::set_label(nmos::resource& resource, const std::string& label)
{

    resource.data[nmos::fields::label] = value::string(utility::s2us(label));
}

void bisect::set_description(nmos::resource& resource, const std::string& description)
{
    resource.data[nmos::fields::description] = value::string(utility::s2us(description));
}

// add an example "natural grouping" hint to a sender or receiver
void bisect::insert_group_hint(nmos::resource& resource, const bisect::port& port)
{
    push_back(resource.data[nmos::fields::tags][nmos::fields::group_hint],
              nmos::make_group_hint({U("example"), resource.type.name + U(' ') + port.name}));
}

nmos::interlace_mode bisect::get_interlace_mode(const nmos::settings& settings)
{
    if(settings.has_field(bisect::fields::interlace_mode))
    {
        return nmos::interlace_mode{bisect::fields::interlace_mode(settings)};
    }
    // for the default, 1080i50 and 1080i59.94 are arbitrarily preferred to 1080p25 and 1080p29.97
    // for 1080i formats, ST 2110-20 says that "the fields of an interlaced image are transmitted in time order,
    // first field first [and] the sample rows of the temporally second field are displaced vertically 'below' the
    // like-numbered sample rows of the temporally first field."
    const auto frame_rate   = nmos::parse_rational(bisect::fields::frame_rate(settings));
    const auto frame_height = bisect::fields::frame_height(settings);
    return (nmos::rates::rate25 == frame_rate || nmos::rates::rate29_97 == frame_rate) && 1080 == frame_height
               ? nmos::interlace_modes::interlaced_tff
               : nmos::interlace_modes::progressive;
}

bisect::maybe_ok bisect::nmoscpp::build_transport_file(const nmos::resources& node_resources,
                                                       nmos_event_handler_t* event_handler,
                                                       const nmos::resource& sender,
                                                       const nmos::resource& connection_sender,
                                                       web::json::value& endpoint_transportfile)
{
    const auto master_enable =
        connection_sender.data.at(nmos::fields::endpoint_staged).at(nmos::fields::master_enable).as_bool();

    if(!master_enable)
    {
        fmt::print("setting transportfile for {} to null\n", utility::us2s(sender.id));
        endpoint_transportfile = web::json::value::null();
        return {};
    }

    fmt::print("setting transportfile for {}\n", utility::us2s(sender.id));

    BST_ASSIGN(info, event_handler->handle_sdp_info_request(sender.id));

    const auto node_id = nmos::find_self_resource(node_resources)->id.c_str();
    const auto node    = nmos::find_resource(node_resources, {node_id, nmos::types::node});

    const auto flow_id = sender.data.at(U("flow_id")).as_string();
    const auto flow    = nmos::find_resource(node_resources, {flow_id, nmos::types::flow});

    if(node_resources.end() == node || node_resources.end() == flow)
    {
        BST_FAIL("matching IS-04 node, flow not found");
    }

    const auto source_id = flow->data.at(U("source_id")).as_string();
    auto source          = nmos::find_resource(node_resources, {source_id, nmos::types::source});

    if(node_resources.end() == source)
    {
        BST_FAIL("matching IS-04 source not found");
    }

    auto params = [&]() -> expected<nmos::sdp_parameters> {
        const std::vector<utility::string_t> mids{U("PRIMARY"), U("SECONDARY")};
        const nmos::format format{nmos::fields::format(flow->data)};
        if(nmos::formats::video == format)
        {
            return nmos::make_video_sdp_parameters(node->data, source->data, flow->data, sender.data, info.payload_type,
                                                   mids, {}, conan_sdp::type_parameters::type_N);
        }
        else if(nmos::formats::audio == format)
        {
            const double packet_time = nmos::fields::channels(source->data).size() > 8 ? 0.125 : 1;
            return nmos::make_audio_sdp_parameters(node->data, source->data, flow->data, sender.data, info.payload_type,
                                                   mids, {}, packet_time);
        }
        else if(nmos::formats::data == format)
        {
            return nmos::make_data_sdp_parameters(node->data, source->data, flow->data, sender.data, info.payload_type,
                                                  mids, {}, {});
        }
        else if(nmos::formats::mux == format)
        {
            return nmos::make_mux_sdp_parameters(node->data, source->data, flow->data, sender.data, info.payload_type,
                                                 mids, {}, conan_sdp::type_parameters::type_N);
        }
        else
        {
            BST_FAIL("unexpected flow format");
        }
    }();
    BST_ASSIGN_MUT(sdp_params, std::move(params));

    auto& transport_params   = nmos::fields::transport_params(nmos::fields::endpoint_active(connection_sender.data));
    auto session_description = nmos::make_session_description(sdp_params, transport_params);
    auto txt                 = conan_sdp::make_session_description(session_description);

    // TODO: this is to overcome a bug that causes the video parameters not to be terminated by "; "
    txt = std::regex_replace(txt, std::regex("; TP=2110TPN"), "; TP=2110TPN; ");

    auto sdp = txt;
    fmt::print("transport file for {} set to: {}\n", utility::us2s(sender.id), sdp);
    endpoint_transportfile = nmos::make_connection_rtp_sender_transportfile(utility::s2us(sdp));

    return {};
}
