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
    config->node.id = "d5504cd1-fe68-489d-99d4-20d3f075f062";
    config->node.configuration_location =
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/config/nmos_plugin_node_config_sender.json";

    // Initialize device_fields_t
    config->device.id          = "e92e628b-7421-4723-9fb9-c1f3b38af9d3";
    config->device.label       = "OSSRF Device2";
    config->device.description = "OSSRF Device2";

    // Initialize video_media_fields_t
    config->video_media_fields.width          = 640;
    config->video_media_fields.height         = 480;
    config->video_media_fields.frame_rate_num = 50;
    config->video_media_fields.frame_rate_den = 1;
    config->video_media_fields.sampling       = "YCbCr-4:2:2";
    config->video_media_fields.structure      = "progressive";

    // Initialize audio_media_fields_t
    config->audio_sender_fields.format             = "audio/L24";
    config->audio_sender_fields.number_of_channels = 1;
    config->audio_sender_fields.packet_time        = 1.000;
    config->audio_sender_fields.sampling_rate      = 48000;

    // Initialize network_fields_t
    config->network.source_address      = "192.168.1.120";
    config->network.interface_name      = "wlp1s0";
    config->network.destination_address = "192.168.1.120";
    config->network.destination_port    = 9999;

    // Initialize config_fields_t
    config->sender_id          = "1c920570-e0b4-4637-b02c-26c9d4275c71";
    config->sender_label       = "BISECT OSSRF Media Sender";
    config->sender_description = "BISECT OSSRF Media Sender";
}

json create_node_config(config_fields_t& config)
{
    const std::string node_config_str = get_node_config(config.node.configuration_location.data());

    if(node_config_str != "")
    {
        json node_config = json::parse(node_config_str);

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

json create_video_sender_config(config_fields_t& config)
{
    json sender = {
        {"id", config.sender_id},
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
         {{"width", config.video_media_fields.width},
          {"height", config.video_media_fields.height},
          {"frame_rate",
           {{"num", config.video_media_fields.frame_rate_num}, {"den", config.video_media_fields.frame_rate_den}}},
          {"sampling", config.video_media_fields.sampling},
          {"structure", config.video_media_fields.structure}}}};
    return sender;
}

json create_audio_sender_config(config_fields_t& config)
{
    std::string media_type = "audio/L16";
    if(config.audio_sender_fields.format == "S24BE")
    {
        media_type = "audio/L24";
    }
    else if(config.audio_sender_fields.format == "S16BE")
    {
        media_type = "audio/L16";
    }
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
                   {"media_type", media_type},
                   {"media",
                    {{"number_of_channels", config.audio_sender_fields.number_of_channels},
                     {"sampling_rate", config.audio_sender_fields.sampling_rate},
                     {"packet_time", config.audio_sender_fields.packet_time}}}};
    return sender;
}

std::string translate_video_format(const std::string& gst_format)
{
    static const std::unordered_map<std::string, std::string> video_format_map = {
        {"UYVP", "YCbCr-4:2:2"}, {"RGB", "RGB-8:8:8"}, {"RGBA", "RGBA-8:8:8:8"}};

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

std::string get_node_config(char* node_configuration_location)
{
    const auto configuration_result = load_configuration_from_file(node_configuration_location);

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
