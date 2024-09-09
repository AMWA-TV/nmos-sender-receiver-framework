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

#include "bisect/nmoscpp/base_nmos_controller.h"
#include "bisect/nmoscpp/detail/internal.h"
#include <nmos/connection_resources.h>
#include <nmos/connection_activation.h>
#include <nmos/connection_api.h>
#include <nmos/settings.h>
#include <nmos/type.h>
#include <nmos/slog.h>
#include <nmos/resource.h>
#include <nmos/resources.h>
#include <nmos/interlace_mode.h>
#include <nmos/json_fields.h>
#include <nmos/channels.h>
#include <nmos/interlace_mode.h>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/join.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <cpprest/host_utils.h>
#include <cpprest/uri.h>

namespace bisect
{
    // the different kinds of 'port' (standing for the format/media type/event type) implemented by the example node
    // each 'port' of the example node has a source, flow, sender and compatible receiver
    DEFINE_STRING_ENUM(port)
    namespace ports
    {
        // video/raw
        const port video{U("v")};
        // audio/L24
        const port audio{U("a")};
        // video/smpte291
        const port data{U("d")};
        // video/SMPTE2022-6
        const port mux{U("m")};

        // example measurement event
        const port temperature{U("t")};
        // example boolean event
        const port burn{U("b")};
        // example string event
        const port nonsense{U("s")};
        // example number/enum event
        const port catcall{U("c")};

        const std::vector<port> rtp{video, audio, data, mux};
        const std::vector<port> ws{temperature, burn, nonsense, catcall};
        const std::vector<port> all{boost::copy_range<std::vector<port>>(boost::range::join(rtp, ws))};
    } // namespace ports

    nmos::id make_id(const nmos::id& seed_id, const nmos::type& type, const port& port = {}, int index = 0);
    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const nmos::type& type, const port& port, int how_many = 1);
    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const nmos::type& type, const std::vector<port>& ports,
                                   int how_many = 1);
    std::vector<nmos::id> make_ids(const nmos::id& seed_id, const std::vector<nmos::type>& types,
                                   const std::vector<port>& ports, int how_many = 1);

    namespace fields
    {
        // how_many: provides for very basic testing of a node with many sub-resources of each type
        const web::json::field_as_integer_or how_many{U("how_many"), 1};

        // activate_senders: controls whether to activate senders on start up (true, default) or not (false)
        const web::json::field_as_bool_or activate_senders{U("activate_senders"), true};

        // frame_rate: controls the grain_rate of video, audio and ancillary data sources and flows
        // and the equivalent parameter constraint on video receivers
        // the value must be an object like { "numerator": 25, "denominator": 1 }
        // hm, unfortunately can't use nmos::make_rational(nmos::rates::rate25) during static initialization
        const web::json::field_as_value_or frame_rate{
            U("frame_rate"), web::json::value_of({{nmos::fields::numerator, 25}, {nmos::fields::denominator, 1}})};

        // frame_width, frame_height: control the frame_width and frame_height of video flows
        const web::json::field_as_integer_or frame_width{U("frame_width"), 1920};
        const web::json::field_as_integer_or frame_height{U("frame_height"), 1080};

        // interlace_mode: controls the interlace_mode of video flows, see nmos::interlace_mode
        // when omitted, a default is used based on the frame_rate, etc.
        const web::json::field_as_string interlace_mode{U("interlace_mode")};

        // channel_count: controls the number of channels in audio sources
        const web::json::field_as_integer_or channel_count{U("channel_count"), 4};

        // smpte2022_7: controls whether senders and receivers have one leg (false) or two legs (true, default)
        const web::json::field_as_bool_or smpte2022_7{U("smpte2022_7"), true};
    } // namespace fields

    const std::vector<nmos::channel> channels_repeat{{U("Left Channel"), nmos::channel_symbols::L},
                                                     {U("Right Channel"), nmos::channel_symbols::R},
                                                     {U("Center Channel"), nmos::channel_symbols::C},
                                                     {U("Low Frequency Effects Channel"), nmos::channel_symbols::LFE}};

    namespace categories
    {
        const nmos::category node_implementation{"node_implementation"};
    }

    std::vector<web::hosts::experimental::host_interface>::const_iterator
    find_interface(const std::vector<web::hosts::experimental::host_interface>& interfaces,
                   const utility::string_t& address);

    void set_label_description(nmos::resource& resource, const bisect::port& port);
    // add an example "natural grouping" hint to a sender or receiver
    void insert_group_hint(nmos::resource& resource, const bisect::port& port);
    nmos::interlace_mode get_interlace_mode(const nmos::settings& settings);
    void set_label(nmos::resource& resource, const std::string& label);
    void set_description(nmos::resource& resource, const std::string& description);
} // namespace bisect

namespace bisect::nmoscpp
{
    nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings,
                                                                                   nmos_event_handler_t* event_handler);

    [[nodiscard]] maybe_ok build_transport_file(const nmos::resources& node_resources,
                                                nmos_event_handler_t* event_handler, const nmos::resource& sender,
                                                const nmos::resource& connection_sender,
                                                web::json::value& endpoint_transportfile);
} // namespace bisect::nmoscpp
