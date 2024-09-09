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

#include "ossrf/nmos/api/nmos_client.h"
#include "ossrf/gstreamer/api/sender/sender_plugin.h"
#include "ossrf/gstreamer/api/receiver/receiver_plugin.h"
#include "bisect/nmoscpp/configuration.h"
#include "bisect/expected/macros.h"
#include "bisect/expected.h"
#include "bisect/json.h"
#include "bisect/initializer.h"
#include <fstream>
#include <future>

using namespace bisect;
using namespace ossrf;
using namespace bisect::nmoscpp;
using json = nlohmann::json;

namespace
{

    void sender_activation_callback(bool master_enabled, const nlohmann::json& transport_params)
    {
        fmt::print("nmos_sender_callback: {} {}\n", master_enabled, transport_params.dump());
    }

    maybe_ok go(const json& app_configuration)
    {
        BST_ASSIGN(node, find<json>(app_configuration, "node"));
        BST_ASSIGN(node_id, find<std::string>(node, "id"));
        BST_ASSIGN(node_configuration, find<json>(node, "configuration"));
        BST_ASSIGN(device, find<json>(app_configuration, "device"));
        BST_ASSIGN(device_id, find<std::string>(device, "id"));

        BST_ASSIGN(nmos_client, nmos_client_t::create(node_id, node_configuration.dump()));
        BST_CHECK(nmos_client->add_device(device.dump()));

        ossrf::gst::plugins::gst_sender_plugin_uptr gst_sender_uptr     = nullptr;
        ossrf::gst::plugins::gst_sender_plugin_uptr gst_sender_uptr_2   = nullptr;
        ossrf::gst::plugins::gst_receiver_plugin_uptr gst_receiver_uptr = nullptr;

        auto receivers_it = app_configuration.find("receivers");
        if(receivers_it != app_configuration.end())
        {
            for(auto it = receivers_it->begin(); it != receivers_it->end(); ++it)
            {
                auto receiver_activation_callback =
                    [r = (*it).dump(), &gst_receiver_uptr](const std::optional<std::string>& sdp, bool master_enable) {
                        if(sdp.has_value())
                        {
                            fmt::print("nmos_receiver_callback: {} {}\n", master_enable, sdp.value());
                            auto plugin = ossrf::gst::plugins::create_gst_receiver_plugin(r, sdp.value());
                            if(plugin.has_value())
                            {
                                gst_receiver_uptr.reset(plugin.value().release());
                                return;
                            }
                            fmt::print("failed creating receiver\n");
                            return;
                        }
                        fmt::print("nmos_receiver_callback: {} no sdp\n", master_enable);
                    };

                BST_CHECK(nmos_client->add_receiver(device_id, (*it).dump(), receiver_activation_callback));
            }
        }

        auto senders_it = app_configuration.find("senders");
        if(senders_it != app_configuration.end())
        {
            auto i = 1;
            for(auto it = senders_it->begin(); it != senders_it->end(); ++it)
            {
                BST_CHECK(nmos_client->add_sender(device_id, (*it).dump(), sender_activation_callback));
                if(i == 1)
                {
                    BST_CHECK_ASSIGN(gst_sender_uptr, ossrf::gst::plugins::create_gst_sender_plugin((*it).dump(), 25));
                }
                else if(i == 2)
                {
                    BST_CHECK_ASSIGN(gst_sender_uptr_2,
                                     ossrf::gst::plugins::create_gst_sender_plugin((*it).dump(), 15));
                }
                i++;
            }
        }

        fmt::print("\n >>> Press a key to stop <<< \n");
        char c;
        std::cin >> c;

        BST_CHECK(nmos_client->remove_resource(device_id, nmos::types::device));
        fmt::print("\n >>> Stopped <<< \n");

        return {};
    }

    expected<json> load_configuration_from_file(std::string_view config_file)
    {
        std::ifstream ifs(config_file.data());
        BST_ENFORCE(ifs.is_open(), "Failed opening file {}", config_file);
        std::ostringstream buffer;
        buffer << ifs.rdbuf();
        return parse_json(buffer.str());
    }

    maybe_ok run(std::string_view configuration_file)
    {
        BST_ASSIGN(configuration, load_configuration_from_file(configuration_file));
        auto init = bisect::gst::initializer();
        return go(configuration);
    }
} // namespace

int main(int argc, char* argv[])
{
    if(argc == 3 && argv[1] == std::string("-f"))
    {
        const auto configuration_file = argv[2];
        auto result                   = run(configuration_file);
        if(!result.has_value())
        {
            fprintf(stderr, "error: %s", result.error().what());
            return -1;
        }
        return 0;
    }

    fprintf(stderr, "usage: %s [-f <configuration file>]\n\n", argv[0]);
    return -1;
}
