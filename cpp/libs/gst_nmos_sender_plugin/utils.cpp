#include "utils.h"
#include "ossrf/nmos/api/nmos_client.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"
#include "bisect/json.h"
#include <memory>

using namespace bisect;
using json = nlohmann::json;

config_fields_t create_default_config_fields()
{
    config_fields_t config;

    // Initialize node_fields_t
    config.node.id = g_strdup("d5504cd1-fe68-489d-99d4-20d3f075f062");
    config.node.configuration_location =
        g_strdup("/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/config/nmos_plugin_node_config.json");

    // Initialize device_fields_t
    config.device.id          = g_strdup("e92e628b-7421-4723-9fb9-c1f3b38af9d3");
    config.device.label       = g_strdup("OSSRF Device2");
    config.device.description = g_strdup("OSSRF Device2");

    // Initialize media_fields_t
    config.media.width              = 640;
    config.media.height             = 480;
    config.media.frame_rate_num     = 50;
    config.media.frame_rate_density = 1;
    config.media.sampling           = g_strdup("YCbCr-4:2:2");
    config.media.structure          = g_strdup("progressive");

    // Initialize network_fields_t
    config.network.source_address      = g_strdup("192.168.1.120");
    config.network.interface_name      = g_strdup("wlp1s0");
    config.network.destination_address = g_strdup("192.168.1.120");
    config.network.destination_port    = g_strdup("5004");

    // Initialize config_fields_t
    config.sender_id          = g_strdup("e543a2c1-d6a2-47f5-8d14-296bb6714ef2");
    config.sender_label       = g_strdup("BISECT OSSRF sender video");
    config.sender_description = g_strdup("BISECT OSSRF sender video");

    return config;
}

json create_node_config(config_fields_t& config)
{
    const std::string node_config_str = get_node_config(config.node.configuration_location);

    // Parse the configuration into a JSON object
    json node_config = json::parse(node_config_str);

    // Override the "id" field with the one from the config_fields_t structure
    node_config["id"] = config.node.id;

    return node_config;
}

json create_device_config(config_fields_t& config)
{
    json device = {
        {"id", config.device.id}, {"label", config.device.label}, {"description", config.device.description}};
    return device;
}

json create_sender_config(config_fields_t& config)
{
    json sender = {{"id", config.sender_id},
                   {"label", config.sender_label},
                   {"description", config.sender_description},
                   {"network",
                    {{"primary",
                      {{"source_address", config.network.source_address},
                       {"interface_name", config.network.interface_name},
                       {"destination_address", config.network.destination_address},
                       {"destination_port", config.network.destination_port}}}}},
                   {"payload_type", 97},
                   {"media_type", "video/raw"},
                   {"media",
                    {{"width", config.media.width},
                     {"height", config.media.height},
                     {"frame_rate", {{"num", config.media.frame_rate_num}, {"den", config.media.frame_rate_density}}},
                     {"sampling", config.media.sampling},
                     {"structure", config.media.structure}}}};
    return sender;
}

expected<json> load_configuration_from_file(std::string_view config_file)
{
    std::ifstream ifs(config_file.data());
    BST_ENFORCE(ifs.is_open(), "Failed opening file {}", config_file);
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    return parse_json(buffer.str());
}

std::string get_node_id(char* node_configuration_location)
{
    const auto configuration_result = load_configuration_from_file(node_configuration_location);

    const json& configuration = configuration_result.value();

    auto node_result = find<json>(configuration, "node");

    const json& node = node_result.value();

    auto node_id_result     = find<std::string>(node, "id");
    auto node_config_result = find<json>(node, "configuration");

    if(node_id_result.has_value() && node_config_result.has_value())
    {
        const std::string node_id            = node_id_result.value();
        const std::string node_configuration = node_config_result.value().dump();

        return node_id;
    }

    return "";
}

std::string get_node_config(char* node_configuration_location)
{
    const auto configuration_result = load_configuration_from_file(node_configuration_location);

    const json& configuration = configuration_result.value();

    auto node_result = find<json>(configuration, "node");

    const json& node = node_result.value();

    auto node_id_result     = find<std::string>(node, "id");
    auto node_config_result = find<json>(node, "configuration");

    if(node_id_result.has_value() && node_config_result.has_value())
    {
        const std::string node_id            = node_id_result.value();
        const std::string node_configuration = node_config_result.value().dump();

        return node_configuration;
    }

    return "";
}

std::string get_device_id(char* device_configuration_location)
{
    const auto configuration_result = load_configuration_from_file(device_configuration_location);

    const json& configuration = configuration_result.value();

    auto device_result = find<json>(configuration, "device");

    const json& device = device_result.value();

    auto device_id_result = find<std::string>(device, "id");

    const std::string device_id     = device_id_result.value();
    const std::string device_config = device.dump();

    return device_id;
}

std::string get_device_config(char* device_configuration_location)
{
    const auto configuration_result = load_configuration_from_file(device_configuration_location);

    const json& configuration = configuration_result.value();

    auto device_result = find<json>(configuration, "device");

    const json& device = device_result.value();

    auto device_id_result = find<std::string>(device, "id");

    const std::string device_id     = device_id_result.value();
    const std::string device_config = device.dump();

    return device_config;
}

std::string get_sender_config(char* sender_configuration_location)
{
    // FIX ME: Gonna be hardcoded for now
    const auto sender_config = load_configuration_from_file(sender_configuration_location);

    return sender_config.value().dump();
}
