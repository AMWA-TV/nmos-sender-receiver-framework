#include "utils.h"
#include "bisect/nmoscpp/base_nmos_controller.h"
#include "bisect/nmoscpp/detail/expected.h"
#include <nmos/connection_events_activation.h>
#include <nmos/sdp_utils.h>
#include <nmos/event_type.h>
#include <nmos/format.h>
#include <nmos/events_resources.h>
#include <nmos/system_resources.h>
#include <sdp/sdp.h>
#include <regex>

using namespace bisect;
using namespace bisect::nmoscpp;
using namespace bisect::core::detail;

namespace
{
    // Example System API node behaviour callback to perform application-specific operations when the global
    // configuration resource changes
    nmos::system_global_handler make_node_implementation_system_global_handler(nmos::node_model& model,
                                                                               slog::base_gate& gate)
    {
        // this example uses the callback to update the settings
        // (an 'empty' std::function disables System API node behaviour)
        return [&](const web::uri& system_uri, const web::json::value& system_global) {
            if(!system_uri.is_empty())
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF)
                    << nmos::stash_category(bisect::categories::node_implementation)
                    << "New system global configuration discovered from the System API at: " << system_uri.to_string();

                // although this example immediately updates the settings, the effect is not propagated
                // in either Registration API behaviour or the senders' /transportfile endpoints until
                // an update to these is forced by other circumstances

                auto system_global_settings = nmos::parse_system_global_data(system_global).second;
                web::json::merge_patch(model.settings, system_global_settings, true);
            }
            else
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF)
                    << nmos::stash_category(bisect::categories::node_implementation)
                    << "System global configuration is not discoverable";
            }
        };
    }

    // Example Registration API node behaviour callback to perform application-specific operations when the current
    // Registration API changes
    nmos::registration_handler make_node_implementation_registration_handler(slog::base_gate& gate)
    {
        return [&](const web::uri& registration_uri) {
            if(!registration_uri.is_empty())
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF)
                    << nmos::stash_category(bisect::categories::node_implementation)
                    << "Started registered operation with Registration API at: " << registration_uri.to_string();
            }
            else
            {
                slog::log<slog::severities::warning>(gate, SLOG_FLF)
                    << nmos::stash_category(bisect::categories::node_implementation) << "Stopped registered operation";
            }
        };
    }

    // Example Connection API callback to parse "transport_file" during a PATCH /staged request
    nmos::transport_file_parser make_node_implementation_transport_file_parser()
    {
        // this example uses the default transport file parser explicitly
        // (if this callback is specified, an 'empty' std::function is not allowed)
        return &nmos::parse_rtp_transport_file;
    }

    // Example Connection API callback to perform application-specific validation of the merged /staged endpoint during
    // a PATCH /staged request
    nmos::details::connection_resource_patch_validator
    make_node_implementation_patch_validator(nmos_event_handler_t* event_handler)
    {
        // this example uses an 'empty' std::function because it does not need to do any validation
        // beyond what is expressed by the schemas and /constraints endpoint
        return [event_handler](const nmos::resource& resource, const nmos::resource& connection_resource,
                               const web::json::value& endpoint_staged, slog::base_gate& gate) {
            auto result = event_handler->handle_patch_request(resource, connection_resource,
                                                              utility::us2s(endpoint_staged.serialize()));

            if(is_error(result))
            {
                throw web::json::json_exception(result.error().what());
            }
        };
    }

    // Example Connection API activation callback to update senders' /transportfile endpoint - captures node_resources
    // by reference!
    nmos::connection_sender_transportfile_setter
    make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings,
                                                  nmos_event_handler_t* event_handler)
    {
        using web::json::value;

        // as part of activation, the sender /transportfile should be updated based on the active transport parameters
        return [&node_resources, event_handler](const nmos::resource& sender, const nmos::resource& connection_sender,
                                                value& endpoint_transportfile) {
            auto result =
                build_transport_file(node_resources, event_handler, sender, connection_sender, endpoint_transportfile);
            if(is_error(result))
            {
                throw std::logic_error(result.error().what());
            }
        };
    }

    // Example Events WebSocket API client message handler
    nmos::events_ws_message_handler make_node_implementation_events_ws_message_handler(const nmos::node_model& model,
                                                                                       slog::base_gate& gate)
    {

        // the message handler will be used for all Events WebSocket connections, and each connection may potentially
        // have subscriptions to a number of sources, for multiple receivers, so this example uses a handler adaptor
        // that enables simple processing of "state" messages (events) per receiver
        return nmos::experimental::make_events_ws_message_handler(
            model,
            [&gate](const nmos::resource& receiver, const nmos::resource& connection_receiver,
                    const web::json::value& message) {
                const auto event_type = nmos::event_type(nmos::fields::state_event_type(message));
                const auto& payload   = nmos::fields::state_payload(message);

                if(nmos::is_matching_event_type(nmos::event_types::wildcard(nmos::event_types::number), event_type))
                {
                    const nmos::events_number value(nmos::fields::payload_number_value(payload).to_double(),
                                                    nmos::fields::payload_number_scale(payload));
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF)
                        << nmos::stash_category(bisect::categories::node_implementation)
                        << "Event received: " << value.scaled_value() << " (" << event_type.name << ")";
                }
                else if(nmos::is_matching_event_type(nmos::event_types::wildcard(nmos::event_types::string),
                                                     event_type))
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF)
                        << nmos::stash_category(bisect::categories::node_implementation)
                        << "Event received: " << nmos::fields::payload_string_value(payload) << " (" << event_type.name
                        << ")";
                }
                else if(nmos::is_matching_event_type(nmos::event_types::wildcard(nmos::event_types::boolean),
                                                     event_type))
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF)
                        << nmos::stash_category(bisect::categories::node_implementation)
                        << "Event received: " << std::boolalpha << nmos::fields::payload_boolean_value(payload) << " ("
                        << event_type.name << ")";
                }
            },
            gate);
    }

    // Example Connection API activation callback to perform application-specific operations to complete activation
    nmos::connection_activation_handler make_node_implementation_connection_activation_handler(nmos::node_model& model,
                                                                                               slog::base_gate& gate)
    {
        auto handle_load_ca_certificates = nmos::make_load_ca_certificates_handler(model.settings, gate);
        // this example uses this callback to (un)subscribe a IS-07 Events WebSocket receiver when it is activated
        // and, in addition to the message handler, specifies the optional close handler in order that any subsequent
        // connection errors are reflected into the /active endpoint by setting master_enable to false
        auto handle_events_ws_message             = make_node_implementation_events_ws_message_handler(model, gate);
        auto handle_close                         = nmos::experimental::make_events_ws_close_handler(model, gate);
        auto connection_events_activation_handler = nmos::make_connection_events_websocket_activation_handler(
            handle_load_ca_certificates, handle_events_ws_message, handle_close, model.settings, gate);

        return [connection_events_activation_handler, &gate](const nmos::resource& resource,
                                                             const nmos::resource& connection_resource) {
            const std::pair<nmos::id, nmos::type> id_type{resource.id, resource.type};
            slog::log<slog::severities::info>(gate, SLOG_FLF)
                << nmos::stash_category(bisect::categories::node_implementation) << "Activating " << id_type;

            connection_events_activation_handler(resource, connection_resource);
        };
    }

    // Example Channel Mapping API callback to perform application-specific validation of the merged active map during a
    // POST /map/activations request
    nmos::details::channelmapping_output_map_validator make_node_implementation_map_validator()
    {
        // this example uses an 'empty' std::function because it does not need to do any validation
        // beyond what is expressed by the schemas and /caps endpoints
        return {};
    }

    // Example Channel Mapping API activation callback to perform application-specific operations to complete activation
    nmos::channelmapping_activation_handler
    make_node_implementation_channelmapping_activation_handler(slog::base_gate& gate)
    {
        return [&gate](const nmos::resource& channelmapping_output) {
            const auto output_id = nmos::fields::channelmapping_id(channelmapping_output.data);
            slog::log<slog::severities::info>(gate, SLOG_FLF)
                << nmos::stash_category(bisect::categories::node_implementation) << "Activating output: " << output_id;
        };
    }

    // This constructs all the callbacks used to integrate the example device-specific underlying implementation
    // into the server instance for the NMOS Node.
    nmos::experimental::node_implementation make_node_implementation(nmos::node_model& model, slog::base_gate& gate,
                                                                     nmos_event_handler_t* event_handler)
    {
        return nmos::experimental::node_implementation()
            .on_load_server_certificates(nmos::make_load_server_certificates_handler(model.settings, gate))
            .on_load_dh_param(nmos::make_load_dh_param_handler(model.settings, gate))
            .on_load_ca_certificates(nmos::make_load_ca_certificates_handler(model.settings, gate))
            .on_system_changed(
                make_node_implementation_system_global_handler(model, gate)) // may be omitted if not required
            .on_registration_changed(
                make_node_implementation_registration_handler(gate)) // may be omitted if not required
            .on_parse_transport_file(
                make_node_implementation_transport_file_parser()) // may be omitted if the default is sufficient
            .on_validate_connection_resource_patch(make_node_implementation_patch_validator(event_handler))
            .on_resolve_auto(make_node_implementation_auto_resolver(model.settings, event_handler))
            .on_set_transportfile(
                make_node_implementation_transportfile_setter(model.node_resources, model.settings, event_handler))
            .on_connection_activated(make_node_implementation_connection_activation_handler(model, gate))
            .on_validate_channelmapping_output_map(
                make_node_implementation_map_validator()) // may be omitted if not required
            .on_channelmapping_activated(make_node_implementation_channelmapping_activation_handler(gate));
    }
} // namespace

nmos_base_controller_t::nmos_base_controller_t(nmos::experimental::log_gate& gate, web::json::value configuration,
                                               nmos_event_handler_t* event_handler)
    : gate_(gate), event_handler_(event_handler)
{
    (void)init(node_model_, gate_, node_implementation_, configuration, event_handler);
}

maybe_ok nmos_base_controller_t::init(nmos::node_model& node_model, nmos::experimental::log_gate& gate,
                                      nmos::experimental::node_implementation& node_implementation,
                                      web::json::value configuration, nmos_event_handler_t* event_handler)
{
    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp node";

        std::error_code error;
        node_model.settings = configuration;
        if(error || !node_model.settings.is_object())
        {
            BST_FAIL("Bad command-line settings [{}]", error.message());
        }
        // Prepare run-time default settings (different from header defaults)

        nmos::insert_node_default_settings(node_model.settings);

        // Log the process ID and initial settings

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Build settings: " << nmos::get_build_settings_info();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << node_model.settings.serialize();

        // Set up the callbacks between the node server and the underlying implementation

        node_implementation = make_node_implementation(node_model, gate, event_handler);

        return {};
    }
    catch(const web::json::json_exception& e)
    {
        // most likely from incorrect types in the command line settings
        BST_FAIL("JSON error: {}", e.what());
    }
    catch(const web::http::http_exception& e)
    {
        BST_FAIL("HTTP error: {} [{}]", e.what(), e.error_code().message());
    }
    catch(const web::websockets::websocket_exception& e)
    {
        BST_FAIL("WebSocket error: {} [{}]", e.what(), e.error_code().message());
    }
    catch(const std::system_error& e)
    {
        BST_FAIL("System error: {} [{}]", e.what(), e.code().message());
    }
    catch(const std::runtime_error& e)
    {
        return std::unexpected(e);
    }
    catch(const std::exception& e)
    {
        BST_FAIL("Unexpected exception: {}", e.what());
    }
    catch(...)
    {
        BST_FAIL("Unexpected unknown exception");
    }
}
