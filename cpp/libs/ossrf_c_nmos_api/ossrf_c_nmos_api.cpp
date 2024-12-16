#include "ossrf_c_nmos_api.h"
#include "ossrf/nmos/api/nmos_client.h"
#include <memory>
#include <string>
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"
#include "bisect/json.h"

using namespace bisect;
using json = nlohmann::json;

struct nmos_client
{
    std::unique_ptr<ossrf::nmos_client_t> client;
    std::string device_id;
    std::unique_ptr<bisect::nmoscpp::sender_activation_callback_t> sender_callback;
    std::unique_ptr<bisect::nmoscpp::receiver_activation_callback_t> receiver_callback;
    std::string sender_info_config;
    std::string receiver_info_config;
};

expected<json> load_configuration_from_file(std::string_view config_file)
{
    std::ifstream ifs(config_file.data());
    BST_ENFORCE(ifs.is_open(), "Failed opening file {}", config_file);
    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    return parse_json(buffer.str());
}

nmos_client_t* nmos_client_create(const char* node_configuration_location)
{
    if(!node_configuration_location)
    {
        return nullptr;
    }

    const auto configuration_result = load_configuration_from_file(node_configuration_location);
    if(!configuration_result.has_value())
    {
        return nullptr;
    }

    const json& configuration = configuration_result.value();

    auto node_result = find<json>(configuration, "node");
    if(!node_result.has_value())
    {
        return nullptr;
    }

    const json& node = node_result.value();

    auto node_id_result     = find<std::string>(node, "id");
    auto node_config_result = find<json>(node, "configuration");

    if(node_id_result.has_value() && node_config_result.has_value())
    {
        const std::string node_id            = node_id_result.value();
        const std::string node_configuration = node_config_result.value().dump();

        auto result = ossrf::nmos_client_t::create(node_id, node_configuration);
        if(!result.has_value())
        {
            return nullptr;
        }

        // wrap result in C compatible structure
        nmos_client_t* wrapper = new nmos_client_t;
        wrapper->client        = std::move(*result);
        return wrapper;
    }

    return nullptr;
}

int nmos_client_add_device(nmos_client_t* wrapper, const char* node_configuration_location)
{
    if(!wrapper || !node_configuration_location)
    {
        return -1;
    }

    const auto configuration_result = load_configuration_from_file(node_configuration_location);
    if(!configuration_result.has_value())
    {
        return -1;
    }

    const json& configuration = configuration_result.value();

    auto device_result = find<json>(configuration, "device");
    if(!device_result.has_value())
    {
        return -1;
    }

    const json& device = device_result.value();

    auto device_id_result = find<std::string>(device, "id");
    if(!device_id_result.has_value())
    {
        return -1;
    }

    const std::string device_id     = device_id_result.value();
    const std::string device_config = device.dump();

    auto result = wrapper->client->add_device(device_config);
    if(!result.has_value())
    {
        return -1;
    }

    wrapper->device_id = device_id;

    return 0;
}

int nmos_client_add_sender(nmos_client_t* wrapper)
{
    auto sender_activation_callback = [](bool master_enabled, const nlohmann::json& transport_params) {
        fmt::print("nmos_sender_callback: {} {}\n", master_enabled, transport_params.dump());
    };
    // FIX ME: Gonna be hardcoded for now
    const auto sender_config = load_configuration_from_file(
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/ossrf-nmos-api/config/nmos_sender_config.json");

    wrapper->client->add_sender(wrapper->device_id, sender_config.value().dump(), sender_activation_callback);
    wrapper->sender_info_config = sender_config.value().dump();

    return 0;
}

int nmos_client_add_receiver(nmos_client_t* wrapper)
{
    auto receiver_activation_callback = [](const std::optional<std::string>& sdp, bool master_enable) {
        fmt::print("nmos_receiver_callback: {} no sdp\n", master_enable);
    };
    // FIX ME: Gonna be hardcoded for now
    const auto receiver_config = load_configuration_from_file(
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/ossrf-nmos-api/config/nmos_receiver_config.json");
    wrapper->client->add_receiver(wrapper->device_id, receiver_config.value().dump(), receiver_activation_callback);
    wrapper->receiver_info_config = receiver_config.value().dump();

    return 0;
}

int nmos_client_remove_sender(nmos_client_t* wrapper)
{
    // FIX ME: Error catching
    wrapper->client->remove_sender(wrapper->device_id, wrapper->sender_info_config);
    return 0;
}

int nmos_client_remove_receiver(nmos_client_t* wrapper)
{
    // FIX ME: Error catching
    wrapper->client->remove_receiver(wrapper->device_id, wrapper->receiver_info_config);
    return 0;
}