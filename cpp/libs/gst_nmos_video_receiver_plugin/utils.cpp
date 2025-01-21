#include "utils.h"
#include "ossrf/nmos/api/nmos_client.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"
#include "bisect/json.h"
#include <memory>

using namespace bisect;
using json = nlohmann::json;

void create_default_config_fields(config_fields_t* config)
{
    if(!config)
    {
        g_critical("Config pointer is NULL");
        return;
    }

    // Initialize node_fields_t
    config->node.id = "d49c85db-1c33-4f21-b160-58edd2af1810";
    config->node.configuration_location =
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/config/nmos_plugin_node_config_receiver.json";

    // Initialize device_fields_t
    config->device.id          = "1ad20d7c-8c58-4c84-8f14-3cf3e3af164c";
    config->device.label       = "OSSRF Device2";
    config->device.description = "OSSRF Device2";

    // Initialize config_fields_t
    config->id             = "db9f46cf-2414-4e25-b6c6-2078159857f9";
    config->label          = "BISECT OSSRF Video Receiver";
    config->description    = "BISECT OSSRF Video Receiver";
    config->interface_name = "wlp1s0";
    config->address        = "192.168.1.36";
}

json create_node_config(config_fields_t& config)
{
    const std::string node_config_str = get_node_config(config.node.configuration_location);
    if(node_config_str != "")
    {
        json node_config  = json::parse(node_config_str);
        node_config["id"] = config.node.id;

        return node_config;
    }
    else
    {
        return nullptr;
    }
}

json create_device_config(config_fields_t& config)
{
    json device = {
        {"id", config.device.id}, {"label", config.device.label}, {"description", config.device.description}};
    return device;
}

json create_receiver_config(config_fields_t& config)
{
    json sender = {
        {"id", config.id},
        {"label", config.label},
        {"description", config.description},
        {"network", {{"primary", {{"interface_address", config.address}, {"interface_name", config.interface_name}}}}},
        {"capabilities", {"video/raw"}}};
    return sender;
}

std::string translate_video_format(const std::string& gst_format)
{
    static const std::unordered_map<std::string, std::string> video_format_map = {{"UYVP", "YCbCr-4:2:2"},
                                                                                  {"RGBA", "RGBA-8:8:8:8"}};

    auto it = video_format_map.find(gst_format);
    if(it != video_format_map.end())
    {
        return it->second;
    }
    return gst_format;
}

std::string translate_audio_format(const std::string& gst_format)
{
    static const std::unordered_map<std::string, std::string> audio_format_map = {{"S24BE", "L24"}, {"S16LE", "L16"}};

    auto it = audio_format_map.find(gst_format);
    if(it != audio_format_map.end())
    {
        return it->second;
    }
    return gst_format;
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

std::string get_node_config(std::string node_configuration_location)
{
    const auto configuration_result = load_configuration_from_file(node_configuration_location.c_str());

    if(configuration_result.has_value())
    {
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
