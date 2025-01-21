
/* GStreamer
 *
 * Copyright (C) <2024> Bisect Lda
 * @author: Luís Ferreira <luis.ferreira@bisect.pt>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "bisect/json.h"
#include "utils.h"
#include "ossrf/nmos/api/nmos_client.h"
#include <gst/gst.h>
#include <gst/gstpad.h>

GST_DEBUG_CATEGORY_STATIC(gst_nmossender_debug_category);
#define GST_CAT_DEFAULT gst_nmossender_debug_category
#define GST_TYPE_NMOSSENDER (gst_nmossender_get_type())
#define GST_NMOSSENDER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NMOSSENDER, GstNmossender))
#define GST_NMOSSENDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NMOSSENDER, GstNmossenderClass))
#define GST_IS_NMOSSENDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NMOSSENDER))
#define GST_IS_NMOSSENDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NMOSSENDER))

typedef struct _GstNmossender
{
    GstBin parent;
    GstElementHandle<_GstElement> queue;
    GstElementHandle<_GstElement> video_payloader;
    GstElementHandle<_GstElement> audio_payloader_16;
    GstElementHandle<_GstElement> audio_payloader_24;
    GstElementHandle<_GstElement> udpsink;
    ossrf::nmos_client_uptr client;
    GstCaps* caps;
    config_fields_t config;
} GstNmossender;

typedef struct _GstNmossenderClass
{
    GstBinClass parent_class;
} GstNmossenderClass;

enum class PropertyId : uint32_t
{
    NodeId                 = 1,
    NodeConfigFileLocation = 2,
    DeviceId               = 3,
    DeviceLabel            = 4,
    DeviceDescription      = 5,
    SenderId               = 6,
    SenderLabel            = 7,
    SenderDescription      = 8,
    SourceAddress          = 9,
    InterfaceName          = 10,
    DestinationAddress     = 11,
    DestinationPort        = 12
};

G_DEFINE_TYPE_WITH_CODE(GstNmossender, gst_nmossender, GST_TYPE_BIN,
                        GST_DEBUG_CATEGORY_INIT(gst_nmossender_debug_category, "nmossender", 0, "NMOS Sender Plugin"))

/* Pad template */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_REQUEST,
                                                                    GST_STATIC_CAPS("video/x-raw, "
                                                                                    "format=(string){ UYVP }; "
                                                                                    "audio/x-raw, "
                                                                                    "format=(string){ S24BE, S16BE }, "
                                                                                    "layout=(string)interleaved, "
                                                                                    "rate=(int)[ 1, 2147483647 ], "
                                                                                    "channels=(int)[ 1, 2147483647 ]"));

/* Set properties so element variables can change depending on them */
static void gst_nmossender_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GstNmossender* self = GST_NMOSSENDER(object);

    switch(static_cast<PropertyId>(property_id))
    {
    case PropertyId::NodeId: self->config.node.id = g_value_dup_string(value); break;

    case PropertyId::NodeConfigFileLocation:
        self->config.node.configuration_location = g_value_dup_string(value);
        break;

    case PropertyId::DeviceId: self->config.device.id = g_value_dup_string(value); break;

    case PropertyId::DeviceLabel: self->config.device.label = g_value_dup_string(value); break;

    case PropertyId::DeviceDescription: self->config.device.description = g_value_dup_string(value); break;

    case PropertyId::SenderId: self->config.sender_id = g_value_dup_string(value); break;

    case PropertyId::SenderLabel: self->config.sender_label = g_value_dup_string(value); break;

    case PropertyId::SenderDescription: self->config.sender_description = g_value_dup_string(value); break;

    case PropertyId::SourceAddress: self->config.network.source_address = g_value_dup_string(value); break;

    case PropertyId::InterfaceName:
        self->config.network.interface_name = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink.get()), "bind_address", self->config.network.interface_name.c_str(),
                     nullptr);
        break;

    case PropertyId::DestinationAddress:
        self->config.network.destination_address = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink.get()), "host", self->config.network.destination_address.c_str(), nullptr);
        break;

    case PropertyId::DestinationPort:
        self->config.network.destination_port = atoi(g_value_get_string(value));
        g_object_set(G_OBJECT(self->udpsink.get()), "port", self->config.network.destination_port, nullptr);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
    }
}

static void gst_nmossender_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GstNmossender* self = GST_NMOSSENDER(object);

    switch(static_cast<PropertyId>(property_id))
    {
    case PropertyId::NodeId: g_value_set_string(value, self->config.node.id.c_str()); break;
    case PropertyId::NodeConfigFileLocation:
        g_value_set_string(value, self->config.node.configuration_location.c_str());
        break;
    case PropertyId::DeviceId: g_value_set_string(value, self->config.device.id.c_str()); break;
    case PropertyId::DeviceLabel: g_value_set_string(value, self->config.device.label.c_str()); break;
    case PropertyId::DeviceDescription: g_value_set_string(value, self->config.device.description.c_str()); break;
    case PropertyId::SenderId: g_value_set_string(value, self->config.sender_id.c_str()); break;
    case PropertyId::SenderLabel: g_value_set_string(value, self->config.sender_label.c_str()); break;
    case PropertyId::SenderDescription: g_value_set_string(value, self->config.sender_description.c_str()); break;
    case PropertyId::SourceAddress: g_value_set_string(value, self->config.network.source_address.c_str()); break;
    case PropertyId::InterfaceName: g_value_set_string(value, self->config.network.interface_name.c_str()); break;
    case PropertyId::DestinationAddress:
        g_value_set_string(value, self->config.network.destination_address.c_str());
        break;
    case PropertyId::DestinationPort: g_value_set_int(value, self->config.network.destination_port); break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
    }
}

/* Event handler for the sink pad */
static gboolean gst_nmossender_sink_event(GstPad* pad, GstObject* parent, GstEvent* event)
{
    GstNmossender* self = GST_NMOSSENDER(parent);

    switch(GST_EVENT_TYPE(event))
    {
    case GST_EVENT_CAPS: {
        GstCaps* caps = nullptr;
        gst_event_parse_caps(event, &caps);

        if(caps)
        {
            gchar* caps_str = gst_caps_to_string(caps);
            // DEBUG
            // g_print("\n\nReceived CAPS from upstream: %s\n", caps_str);

            const GstStructure* structure = gst_caps_get_structure(caps, 0);
            std::string media_type        = gst_structure_get_name(structure);

            if(media_type == "audio/x-raw")
            {

                self->caps            = gst_caps_ref(caps);
                self->config.is_audio = true;
                for(guint i = 0; i < gst_caps_get_size(caps); i++)
                {
                    gint rate, channels;
                    const gchar* format;
                    if(gst_structure_get_int(structure, "rate", &rate))
                    {
                        self->config.audio_sender_fields.sampling_rate = rate;
                    }
                    if(gst_structure_get_int(structure, "channels", &channels))
                    {
                        self->config.audio_sender_fields.number_of_channels = channels;
                    }
                    if((format = gst_structure_get_string(structure, "format")))
                    {
                        self->config.audio_sender_fields.format = format;
                    }
                }
                // DEBUG
                // g_print("Audio caps detected\n");
                // GST_INFO_OBJECT(self, "Audio caps detected");
                //
                if(self->config.audio_sender_fields.format == "S24BE")
                {
                    if(gst_element_link(self->queue.get(), self->audio_payloader_24.get()) == false)
                    {
                        GST_ERROR_OBJECT(self, "Failed to link queue to audio_payloader");
                        return false;
                    }
                    if(gst_element_link(self->audio_payloader_24.get(), self->udpsink.get()) == false)
                    {
                        GST_ERROR_OBJECT(self, "Failed to link audio_payloader_24 to udpsink");
                        return false;
                    }
                }
                else
                {
                    if(gst_element_link(self->queue.get(), self->audio_payloader_16.get()) == false)
                    {
                        GST_ERROR_OBJECT(self, "Failed to link queue to audio_payloader");
                        return false;
                    }
                    if(gst_element_link(self->audio_payloader_16.get(), self->udpsink.get()) == false)
                    {
                        GST_ERROR_OBJECT(self, "Failed to link audio_payloader_16 to udpsink");
                        return false;
                    }
                }
                // DEBUG
                // g_print("Rate: %d, Channels: %d, Format: %s\n", self->config.audio_sender_fields.sampling_rate,
                //         self->config.audio_sender_fields.number_of_channels,
                //         self->config.audio_sender_fields.format);
            }
            else if(media_type == "video/x-raw")
            {
                // DEBUG
                // g_print("Video caps detected\n");
                // GST_INFO_OBJECT(self, "Video caps detected");
                //
                if(gst_element_link(self->queue.get(), self->video_payloader.get()) == false)
                {
                    GST_ERROR_OBJECT(self, "Failed to link queue to video_payloader");
                    return false;
                }
                if(gst_element_link(self->video_payloader.get(), self->udpsink.get()) == false)
                {
                    GST_ERROR_OBJECT(self, "Failed to link video_payloader to udpsink");
                    return false;
                }
                self->caps            = gst_caps_ref(caps);
                self->config.is_audio = false;
                for(guint i = 0; i < gst_caps_get_size(caps); i++)
                {
                    gint width, height;
                    const gchar* format;
                    if(gst_structure_get_int(structure, "width", &width))
                    {
                        self->config.video_media_fields.width = width;
                    }
                    if(gst_structure_get_int(structure, "height", &height))
                    {
                        self->config.video_media_fields.height = height;
                    }
                    if((format = gst_structure_get_string(structure, "format")))
                    {
                        self->config.video_media_fields.sampling = translate_video_format(format);
                    }
                }

                // DEBUG
                // g_print("Width: %d, Height: %d, Format: %s\n", self->config.video_media_fields.width,
                //         self->config.video_media_fields.height, self->config.video_media_fields.sampling);
            }
            else
            {
                GST_WARNING_OBJECT(self, "Unsupported media type: %s", media_type.c_str());
            }

            g_free(caps_str);
        }
        else
        {
            GST_WARNING_OBJECT(self, "No caps found in CAPS event");
        }

        break;
    }
    default: break;
    }
    // Pass the event downstream
    return gst_pad_event_default(pad, parent, event);
}

/* State Change so it doesn't boot NMOS without pipeline being set to playing */
static GstStateChangeReturn gst_nmossender_change_state(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret;
    GstNmossender* self = GST_NMOSSENDER(element);

    auto sender_activation_callback = [](bool master_enabled, const nlohmann::json& transport_params) {
        fmt::print("nmos_sender_callback: master_enabled={}, transport_params={}\n", master_enabled,
                   transport_params.dump());
    };

    switch(transition)
    {
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING: {

        const auto node_config_json = create_node_config(self->config);
        if(node_config_json == nullptr)
        {
            GST_ERROR_OBJECT(self, "Failed to initialize NMOS client. No valid node JSON location given.");
            return GST_STATE_CHANGE_FAILURE;
        }
        const auto device_config_json                       = create_device_config(self->config);
        nlohmann::json_abi_v3_11_3::json sender_config_json = nullptr;

        if(self->config.is_audio)
        {
            sender_config_json = create_audio_sender_config(self->config);
        }
        else
        {
            sender_config_json = create_video_sender_config(self->config);
        }

        // DEBUG
        // g_print("Node Configuration: %s\n", node_config_json.dump().c_str());
        // g_print("Device Configuration: %s\n", device_config_json.dump().c_str());
        // g_print("Sender Configuration: %s\n", sender_config_json.dump().c_str());

        auto result = ossrf::nmos_client_t::create(self->config.node.id, node_config_json.dump());
        if(result.has_value() == false)
        {
            GST_ERROR_OBJECT(self, "Failed to initialize NMOS client. Node ID: %s", self->config.node.id.c_str());
            return GST_STATE_CHANGE_FAILURE;
        }

        self->client = std::move(result.value());
        if(!self->client->add_device(device_config_json.dump()))
        {
            GST_ERROR_OBJECT(self, "Failed to add device to NMOS client");
            return GST_STATE_CHANGE_FAILURE;
        }

        if(!self->client->add_sender(self->config.device.id, sender_config_json.dump(), sender_activation_callback))
        {
            GST_ERROR_OBJECT(self, "Failed to add sender to NMOS client");
            return GST_STATE_CHANGE_FAILURE;
        }
    }
    break;

    default: break;
    }

    // This is needed to work ¯\_(ツ)_/¯
    ret = GST_ELEMENT_CLASS(gst_nmossender_parent_class)->change_state(element, transition);

    return ret;
}

/* Helper function to add properties */
static void add_property(GObjectClass* object_class, guint id, const gchar* name, const gchar* nick, const gchar* blurb,
                         const gchar* default_value)
{
    g_object_class_install_property(object_class, id,
                                    g_param_spec_string(name, nick, blurb, default_value,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

/* Class initialization */
static void gst_nmossender_class_init(GstNmossenderClass* klass)
{
    GObjectClass* object_class     = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_template));

    object_class->set_property = gst_nmossender_set_property;
    object_class->get_property = gst_nmossender_get_property;

    // Add properties using helper function
    add_property(object_class, 1, "node-id", "Node ID", "The ID of the node", "0aad3458-1081-4fba-af02-a8ebd9feeae3");
    add_property(
        object_class, 2, "node-config-file-location", "Node Config File Location",
        "The location of the configuration file for the node",
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/ossrf-nmos-api/config/nmos_plugin_config.json");
    add_property(object_class, 3, "device-id", "Device ID", "The ID of the device",
                 "b9b85f97-58db-41fe-934f-c2afbf7bd46f");
    add_property(object_class, 4, "device-label", "Device Label", "The label to identify the device", "OSSRF Device");
    add_property(object_class, 5, "device-description", "Device Description", "Description of the device",
                 "OSSRF Device Description");
    add_property(object_class, 6, "sender-id", "Sender ID", "The ID of the sender",
                 "e543a2c1-d6a2-47f5-8d14-296bb6714ef2");
    add_property(object_class, 7, "sender-label", "Sender Label", "Label to identify the sender",
                 "BISECT sender video");
    add_property(object_class, 8, "sender-description", "Sender Description",
                 "Description string to better describe the sender", "BISECT sender video");
    add_property(object_class, 9, "source-address", "Source Address", "The address of the source", "127.0.0.1");
    add_property(object_class, 10, "interface-name", "Interface Name", "Name of the interface", "wlp1s0");
    add_property(object_class, 11, "destination-address", "Destination Address", "The address of the destination",
                 "127.0.0.1");
    add_property(object_class, 12, "destination-port", "Destination Port", "Port of the destination", "9999");

    gst_element_class_set_static_metadata(element_class, "NMOS Sender", "Sink/Network",
                                          "Processes raw video and sends it over RTP and UDP to NMOS client",
                                          "Luis Ferreira <luis.ferreira@bisect.pt>");

    element_class->change_state = gst_nmossender_change_state;
}

/* Object initialization */
static void gst_nmossender_init(GstNmossender* self)
{
    auto maybeQueue      = GstElementHandle<GstElement>::create_element("queue", nullptr);
    auto maybeVideoPay   = GstElementHandle<GstElement>::create_element("rtpvrawpay", nullptr);
    auto maybeAudioPay16 = GstElementHandle<GstElement>::create_element("rtpL16pay", nullptr);
    auto maybeAudioPay24 = GstElementHandle<GstElement>::create_element("rtpL24pay", nullptr);
    auto maybeUdpSink    = GstElementHandle<GstElement>::create_element("udpsink", nullptr);

    if(std::holds_alternative<std::nullptr_t>(maybeQueue) || std::holds_alternative<std::nullptr_t>(maybeVideoPay) ||
       std::holds_alternative<std::nullptr_t>(maybeAudioPay16) ||
       std::holds_alternative<std::nullptr_t>(maybeAudioPay24) || std::holds_alternative<std::nullptr_t>(maybeUdpSink))
    {
        GST_ERROR_OBJECT(self, "Failed to create pipeline elements.");
        return;
    }

    GstElementHandle<GstElement> queue      = std::move(std::get<GstElementHandle<GstElement>>(maybeQueue));
    GstElementHandle<GstElement> rtpvrawpay = std::move(std::get<GstElementHandle<GstElement>>(maybeVideoPay));
    GstElementHandle<GstElement> rtpL16pay  = std::move(std::get<GstElementHandle<GstElement>>(maybeAudioPay16));
    GstElementHandle<GstElement> rtpL24pay  = std::move(std::get<GstElementHandle<GstElement>>(maybeAudioPay24));
    GstElementHandle<GstElement> udpsink    = std::move(std::get<GstElementHandle<GstElement>>(maybeUdpSink));

    self->queue              = std::move(queue);
    self->video_payloader    = std::move(rtpvrawpay);
    self->audio_payloader_16 = std::move(rtpL16pay);
    self->audio_payloader_24 = std::move(rtpL24pay);
    self->udpsink            = std::move(udpsink);

    // set properties
    g_object_set(G_OBJECT(self->queue.get()), "max-size-buffers", 1, nullptr);
    g_object_set(G_OBJECT(self->udpsink.get()), "host", "127.0.0.1", "port", 9999, nullptr);
    create_default_config_fields(&self->config);

    gst_bin_add_many(GST_BIN(self), self->queue.get(), self->video_payloader.get(), self->audio_payloader_24.get(),
                     self->audio_payloader_16.get(), self->udpsink.get(), nullptr);

    // create and configure the sink pad
    GstPad* queue_sinkpad = gst_element_get_static_pad(self->queue.get(), "sink");
    if(queue_sinkpad == nullptr)
    {
        GST_ERROR_OBJECT(self, "Failed to get static pad 'sink' from queue");
        return;
    }
    GstPad* sink_ghost_pad = gst_ghost_pad_new("sink", queue_sinkpad);
    gst_object_unref(queue_sinkpad);

    if(sink_ghost_pad == nullptr)
    {
        GST_ERROR_OBJECT(self, "Failed to create ghost pad for sink");
        return;
    }

    gst_pad_set_event_function(sink_ghost_pad, gst_nmossender_sink_event);

    if(gst_element_add_pad(GST_ELEMENT(self), sink_ghost_pad) == false)
    {
        GST_ERROR_OBJECT(self, "Failed to add ghost pad to element; pad with same name might exist");
        gst_object_unref(sink_ghost_pad);
        return;
    }
}

static gboolean plugin_init(GstPlugin* plugin)
{
    return gst_element_register(plugin, "nmossender", GST_RANK_NONE, GST_TYPE_NMOSSENDER);
}

#define VERSION "0.1"
#define PACKAGE "gst-nmos-sender-plugin"
#define PACKAGE_NAME "AMWA NMOS Sender and Receiver Framework Plugins"
#define GST_PACKAGE_ORIGIN "https://www.amwa.tv/"
#define GST_PACKAGE_LICENSE "Apache-2.0"

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, nmossender, "Plugin to send to NMOS application video stream",
                  plugin_init, VERSION, GST_PACKAGE_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
