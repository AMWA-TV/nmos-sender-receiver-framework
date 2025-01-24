#include "utils.h"
#include "ossrf/nmos/api/nmos_client.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"
#include "bisect/json.h"
#include "gst_nmos_plugins/include/element_class.h"
#include "gst_nmos_plugins/include/nmos_configuration.h"
#include <memory>

using namespace bisect;
using json = nlohmann::json;

void create_default_config_fields_sender(config_fields_t* config)
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
    config->id          = "1c920570-e0b4-4637-b02c-26c9d4275c71";
    config->label       = "BISECT OSSRF Media Sender";
    config->description = "BISECT OSSRF Media Sender";
}

void create_default_config_fields_video_receiver(config_fields_t* config)
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
    config->is_audio       = false;
}

void create_default_config_fields_audio_receiver(config_fields_t* config)
{
    if(!config)
    {
        g_critical("Config pointer is NULL");
        return;
    }

    // Initialize node_fields_t
    config->node.id = "d1073007-4099-452f-b8e8-837b8987d845";
    config->node.configuration_location =
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/config/nmos_plugin_node_config_receiver.json";

    // Initialize device_fields_t
    config->device.id          = "093a1c56-4033-49f7-9e17-fbcb21a19383";
    config->device.label       = "OSSRF Device2";
    config->device.description = "OSSRF Device2";

    // Initialize config_fields_t
    config->id             = "af600517-a452-4c2c-8b60-2caa770d6435";
    config->label          = "BISECT OSSRF Audio Receiver";
    config->description    = "BISECT OSSRF Audio Receiver";
    config->interface_name = "wlp1s0";
    config->address        = "192.168.1.36";
    config->is_audio       = true;
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
    std::string caps = "video/raw";
    if(config.is_audio == true)
    {
        caps = "audio/L24";
    }
    json sender = {
        {"id", config.id},
        {"label", config.label},
        {"description", config.description},
        {"network", {{"primary", {{"interface_address", config.address}, {"interface_name", config.interface_name}}}}},
        {"capabilities", {caps}}};
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

json create_video_sender_config(config_fields_t& config)
{
    json sender = {
        {"id", config.id},
        {"label", config.label},
        {"description", config.description},
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
    json sender = {{"id", config.id},
                   {"label", config.label},
                   {"description", config.description},
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
