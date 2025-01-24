
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

#include "bisect/expected/macros.h"
#include "bisect/json.h"
#include "bisect/sdp.h"
#include "bisect/sdp/reader.h"
#include "bisect/nmoscpp/configuration.h"
#include "utils.h"
#include "ossrf/nmos/api/nmos_client.h"
#include "../include/element_class.h"
#include "../include/nmos_configuration.h"
#include <gst/gst.h>
#include <gst/gstpad.h>

using namespace bisect;
using namespace bisect::sdp;

GST_DEBUG_CATEGORY_STATIC(gst_nmosaudioreceiver_debug_category);
#define GST_CAT_DEFAULT gst_nmosaudioreceiver_debug_category
#define GST_TYPE_NMOSAUDIORECEIVER (gst_nmosaudioreceiver_get_type())
#define GST_NMOSAUDIORECEIVER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NMOSAUDIORECEIVER, GstNmosaudioreceiver))
#define GST_NMOSAUDIORECEIVER_CLASS(klass)                                                                             \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NMOSAUDIORECEIVER, GstNmosaudioreceiverClass))
#define GST_IS_NMOSAUDIORECEIVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NMOSAUDIORECEIVER))
#define GST_IS_NMOSAUDIORECEIVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NMOSAUDIORECEIVER))

typedef struct _GstNmosaudioreceiver
{
    GstBin parent;
    GstPad* element_pad;
    GstPad* bin_pad;
    GstClock* clock;
    GstElementHandle<_GstElement> bin;
    GstElementHandle<_GstElement> udp_src;
    GstElementHandle<_GstElement> rtp_audio_depay_16;
    GstElementHandle<_GstElement> rtp_audio_depay_24;
    GstElementHandle<_GstElement> rtp_jitter_buffer;
    GstElementHandle<_GstElement> queue;
    ossrf::nmos_client_uptr client;
    config_fields_t config;
    std::string sdp_string;
    sdp_settings_t sdp_settings;
    bool nmos_active;
} GstNmosaudioreceiver;

typedef struct _GstNmosaudioreceiverClass
{
    GstBinClass parent_class;
} GstNmosaudioreceiverClass;

G_DEFINE_TYPE_WITH_CODE(GstNmosaudioreceiver, gst_nmosaudioreceiver, GST_TYPE_BIN,
                        GST_DEBUG_CATEGORY_INIT(gst_nmosaudioreceiver_debug_category, "nmosaudioreceiver", 0,
                                                "NMOS Receiver Plugin"))
// Pad template
static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("audio/x-raw, "
                                                                                   "format=(string){ S16BE, S24BE }; "
                                                                                   "application/x-rtp"));

enum class PropertyId : uint32_t
{
    NodeId                 = 1,
    NodeConfigFileLocation = 2,
    DeviceId               = 3,
    DeviceLabel            = 4,
    DeviceDescription      = 5,
    ReceiverId             = 6,
    ReceiverLabel          = 7,
    ReceiverDescription    = 8,
    DstAddress             = 9
};

// Set properties so element variables can change depending on them
static void gst_nmosaudioreceiver_set_property(GObject* object, guint property_id, const GValue* value,
                                               GParamSpec* pspec)
{
    GstNmosaudioreceiver* self = GST_NMOSAUDIORECEIVER(object);

    switch(static_cast<PropertyId>(property_id))
    {
    case PropertyId::NodeId: self->config.node.id = g_value_dup_string(value); break;

    case PropertyId::NodeConfigFileLocation:
        self->config.node.configuration_location = g_value_dup_string(value);
        break;

    case PropertyId::DeviceId: self->config.device.id = g_value_dup_string(value); break;

    case PropertyId::DeviceLabel: self->config.device.label = g_value_dup_string(value); break;

    case PropertyId::DeviceDescription: self->config.device.description = g_value_dup_string(value); break;

    case PropertyId::ReceiverId: self->config.id = g_value_dup_string(value); break;

    case PropertyId::ReceiverLabel: self->config.label = g_value_dup_string(value); break;

    case PropertyId::ReceiverDescription: self->config.description = g_value_dup_string(value); break;

    case PropertyId::DstAddress: self->config.address = g_value_dup_string(value); break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
    }
}

static void gst_nmosaudioreceiver_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GstNmosaudioreceiver* self = GST_NMOSAUDIORECEIVER(object);

    switch(static_cast<PropertyId>(property_id))
    {

    case PropertyId::NodeId: g_value_set_string(value, self->config.node.id.c_str()); break;
    case PropertyId::NodeConfigFileLocation:
        g_value_set_string(value, self->config.node.configuration_location.c_str());
        break;
    case PropertyId::DeviceId: g_value_set_string(value, self->config.device.id.c_str()); break;
    case PropertyId::DeviceLabel: g_value_set_string(value, self->config.device.label.c_str()); break;
    case PropertyId::DeviceDescription: g_value_set_string(value, self->config.device.description.c_str()); break;
    case PropertyId::ReceiverId: g_value_set_string(value, self->config.id.c_str()); break;
    case PropertyId::ReceiverLabel: g_value_set_string(value, self->config.label.c_str()); break;
    case PropertyId::ReceiverDescription: g_value_set_string(value, self->config.description.c_str()); break;
    case PropertyId::DstAddress: g_value_set_string(value, self->config.address.c_str()); break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
    }
}

static GstPadProbeReturn block_pad_probe_cb(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    return GST_PAD_PROBE_DROP;
}

void remove_old_bin(GstNmosaudioreceiver* self)
{
    if(self->element_pad == nullptr)
    {
        return;
    }
    gulong block_id =
        gst_pad_add_probe(self->element_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, block_pad_probe_cb, NULL, NULL);

    gst_element_send_event(GST_ELEMENT(self), gst_event_new_flush_start());
    gst_element_send_event(GST_ELEMENT(self), gst_event_new_flush_stop(FALSE));

    self->clock = gst_element_get_clock(GST_ELEMENT(self));

    gst_element_set_state(self->bin.get(), GST_STATE_NULL);

    gst_bin_remove(GST_BIN(self), self->udp_src.get());
    gst_bin_remove(GST_BIN(self), self->rtp_jitter_buffer.get());
    gst_bin_remove(GST_BIN(self), self->rtp_audio_depay_16.get());
    gst_bin_remove(GST_BIN(self), self->rtp_audio_depay_24.get());
    gst_bin_remove(GST_BIN(self), self->queue.get());

    gst_bin_remove(GST_BIN(self), self->bin.get());

    self->udp_src.forget();
    self->queue.forget();
    self->rtp_jitter_buffer.forget();
    self->rtp_audio_depay_16.forget();
    self->rtp_audio_depay_24.forget();
    self->bin.forget();

    if(block_id != 0)
    {
        gst_pad_remove_probe(self->element_pad, block_id);
    }
}

void construct_pipeline(GstNmosaudioreceiver* self)
{
    auto format = self->sdp_settings.format;
    try
    {
        bisect::nmoscpp::audio_sender_info_t audio_info = std::get<bisect::nmoscpp::audio_sender_info_t>(format);
        fmt::print("\n===== SDP Settings =====\n"
                   "Audio (from variant):\n"
                   "  - bits per sample: {}\n"
                   "  - number of channels:  {}\n"
                   "  - packet time: {}\n"
                   "  - sampling rate: {}\n"
                   "Primary leg:\n"
                   "  - destination_ip:   {}\n"
                   "  - destination_port: {}\n"
                   "\n",
                   audio_info.bits_per_sample, audio_info.number_of_channels, audio_info.packet_time,
                   audio_info.sampling_rate, self->sdp_settings.primary.destination_ip.value().c_str(),
                   self->sdp_settings.primary.destination_port.value());

        auto maybeBin = GstElementHandle<GstElement>::create_bin("dynamic-bin");
        if(std::holds_alternative<std::nullptr_t>(maybeBin))
        {
            GST_ERROR_OBJECT(self, "Failed to create bin");
            return;
        }

        self->bin = std::move(std::get<GstElementHandle<GstElement>>(maybeBin));

        auto maybeUdpsrc  = GstElementHandle<GstElement>::create_element("udpsrc", nullptr);
        auto maybeQueue   = GstElementHandle<GstElement>::create_element("queue", nullptr);
        auto maybeJitter  = GstElementHandle<GstElement>::create_element("rtpjitterbuffer", nullptr);
        auto maybeDepay16 = GstElementHandle<GstElement>::create_element("rtpL16depay", nullptr);
        auto maybeDepay24 = GstElementHandle<GstElement>::create_element("rtpL24depay", nullptr);

        if(std::holds_alternative<std::nullptr_t>(maybeUdpsrc) || std::holds_alternative<std::nullptr_t>(maybeQueue) ||
           std::holds_alternative<std::nullptr_t>(maybeJitter) ||
           std::holds_alternative<std::nullptr_t>(maybeDepay16) || std::holds_alternative<std::nullptr_t>(maybeDepay24))
        {
            GST_ERROR_OBJECT(self, "Failed to create pipeline elements.");
            return;
        }

        self->udp_src            = std::move(std::get<GstElementHandle<GstElement>>(maybeUdpsrc));
        self->queue              = std::move(std::get<GstElementHandle<GstElement>>(maybeQueue));
        self->rtp_jitter_buffer  = std::move(std::get<GstElementHandle<GstElement>>(maybeJitter));
        self->rtp_audio_depay_16 = std::move(std::get<GstElementHandle<GstElement>>(maybeDepay16));
        self->rtp_audio_depay_24 = std::move(std::get<GstElementHandle<GstElement>>(maybeDepay24));

        g_object_set(
            G_OBJECT(self->udp_src.get()), "address", self->sdp_settings.primary.destination_ip.value().c_str(), "port",
            self->sdp_settings.primary.destination_port.value(), "do-timestamp", true, "buffer-size", 212992, nullptr);

        GstCaps* caps = gst_caps_new_simple("application/x-rtp", "clock-rate", G_TYPE_INT, audio_info.sampling_rate,
                                            "channels", G_TYPE_INT, audio_info.number_of_channels, nullptr);

        g_object_set(G_OBJECT(self->udp_src.get()), "caps", caps, nullptr);

        g_object_set(G_OBJECT(self->rtp_jitter_buffer.get()), "do-retransmission", true, "do-lost", true, "mode", 0,
                     "latency", 0, nullptr);

        g_object_set(G_OBJECT(self->queue.get()), "max-size-buffers", 3, nullptr);

        gst_bin_add_many(GST_BIN(self->bin.get()), self->udp_src.get(), self->rtp_jitter_buffer.get(),
                         self->queue.get(), self->rtp_audio_depay_16.get(), self->rtp_audio_depay_24.get(), nullptr);

        gst_element_sync_state_with_parent(self->udp_src.get());
        gst_element_sync_state_with_parent(self->rtp_jitter_buffer.get());
        gst_element_sync_state_with_parent(self->queue.get());
        gst_element_sync_state_with_parent(self->rtp_audio_depay_16.get());
        gst_element_sync_state_with_parent(self->rtp_audio_depay_24.get());

        GstPad* bin_src_pad = nullptr;

        if(audio_info.bits_per_sample == 16)
        {

            if(gst_element_link_many(self->udp_src.get(), self->rtp_jitter_buffer.get(), self->queue.get(),
                                     self->rtp_audio_depay_16.get(), nullptr) == false)
            {
                GST_ERROR_OBJECT(self, "Failed to link elements inside the bin.");
                return;
            }

            // Create a ghost pad from the depay's src
            bin_src_pad = gst_element_get_static_pad(self->rtp_audio_depay_16.get(), "src");
            if(bin_src_pad == nullptr)
            {
                GST_ERROR_OBJECT(self, "Failed to get src pad from rtp_audio_depay.");
                return;
            }
        }
        else
        {
            if(gst_element_link_many(self->udp_src.get(), self->rtp_jitter_buffer.get(), self->queue.get(),
                                     self->rtp_audio_depay_24.get(), nullptr) == false)
            {
                GST_ERROR_OBJECT(self, "Failed to link elements inside the bin.");
                return;
            }

            // Create a ghost pad from the depay's src
            bin_src_pad = gst_element_get_static_pad(self->rtp_audio_depay_24.get(), "src");
            if(bin_src_pad == nullptr)
            {
                GST_ERROR_OBJECT(self, "Failed to get src pad from rtp_audio_depay.");
                return;
            }
        }

        GstPad* bin_ghost_pad = gst_ghost_pad_new("src", bin_src_pad);
        gst_object_unref(bin_src_pad);
        self->bin_pad = bin_ghost_pad;

        if(bin_ghost_pad == nullptr || gst_element_add_pad(self->bin.get(), bin_ghost_pad) == false)
        {
            GST_ERROR_OBJECT(self, "Failed to create or add ghost pad to bin.");
            return;
        }

        gst_bin_add(GST_BIN(self), self->bin.get());

        GstPad* plugin_pad = gst_element_get_static_pad(GST_ELEMENT(self), "src");
        if(plugin_pad == nullptr)
        {
            GST_ERROR_OBJECT(self, "Failed to get plugin ghost pad.");
            return;
        }

        self->element_pad = plugin_pad;

        if(gst_ghost_pad_set_target(GST_GHOST_PAD(self->element_pad), bin_ghost_pad) == false)
        {
            GST_ERROR_OBJECT(self, "Failed to link plugin ghost pad to bin's ghost pad.");
            return;
        }

        gst_object_unref(plugin_pad);
    }
    catch(std::bad_variant_access const& ex)
    {
        fmt::print("Invalid format sent for current receiver.\n");
    }
}
void create_nmos(GstNmosaudioreceiver* self)
{
    auto initialize_nmos = [&]() -> bool {
        const auto node_config_json = create_node_config(self->config);
        if(node_config_json == nullptr)
        {
            GST_ERROR_OBJECT(self, "Failed to initialize NMOS client. No valid node JSON location given.");
            return false;
        }

        auto result = ossrf::nmos_client_t::create(self->config.node.id, node_config_json.dump());
        if(!result.has_value())
        {
            GST_ERROR_OBJECT(self, "Failed to initialize NMOS client.");
            return false;
        }

        self->client = std::move(result.value());
        return true;
    };

    if(!initialize_nmos()) return;

    // Add device and receiver configurations
    self->client->add_device(create_device_config(self->config).dump());
    self->client->add_receiver(
        self->config.device.id, create_receiver_config(self->config).dump(),
        [self](const std::optional<std::string>& sdp, bool master_enabled) {
            fmt::print("Receiver Activation Callback: SDP={}, Master Enabled={}\n",
                       sdp.has_value() ? sdp.value() : "None", master_enabled);

            if(master_enabled)
            {
                if(sdp)
                {
                    fmt::print("Received SDP: {}\n", sdp.value());
                    auto sdp_settings = parse_sdp(sdp.value());
                    if(sdp_settings.has_value() && sdp.value() != self->sdp_string)
                    {
                        self->sdp_settings = sdp_settings.value();
                        self->sdp_string   = sdp.value();
                        remove_old_bin(self);
                        construct_pipeline(self);
                        auto time = gst_element_get_base_time(GST_ELEMENT(self));
                        gst_element_set_state(GST_ELEMENT(self), GST_STATE_PLAYING);
                        gst_pad_set_offset(self->bin_pad, static_cast<long>(gst_clock_get_time(self->clock) - time));
                    }
                }
                else if(!sdp && self->sdp_string != "")
                {
                    fmt::print("No new SDP provided, enabling master pipeline.\n");
                    remove_old_bin(self);
                    construct_pipeline(self);
                    auto time = gst_element_get_base_time(GST_ELEMENT(self));
                    gst_element_set_state(GST_ELEMENT(self), GST_STATE_PLAYING);
                    gst_pad_set_offset(self->bin_pad, static_cast<long>(gst_clock_get_time(self->clock) - time));
                }
            }
            else
            {
                if(sdp)
                {
                    fmt::print("Disabling master: SDP received but master is not enabled.\n");
                    remove_old_bin(self);
                }
                else
                {
                    fmt::print("Master not enabled and no SDP provided.\n");
                }
            }
            fmt::print("Master enabled: {}\n", master_enabled);
        });

    GST_INFO_OBJECT(self, "NMOS client initialized successfully.");
}

static GstStateChangeReturn gst_nmosaudioreceiver_change_state(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret;
    GstNmosaudioreceiver* self = GST_NMOSAUDIORECEIVER(element);

    switch(transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY: {
        if(self->nmos_active != true)
        {
            create_nmos(self);
            self->nmos_active = true;
        }
    }
    break;

    default: break;
    }

    ret = GST_ELEMENT_CLASS(gst_nmosaudioreceiver_parent_class)->change_state(element, transition);

    return ret;
}

static void add_property(GObjectClass* object_class, guint id, const gchar* name, const gchar* nick, const gchar* blurb,
                         const gchar* default_value)
{
    g_object_class_install_property(object_class, id,
                                    g_param_spec_string(name, nick, blurb, default_value,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

// Class initialization
static void gst_nmosaudioreceiver_class_init(GstNmosaudioreceiverClass* klass)
{
    GObjectClass* object_class     = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_template));

    object_class->set_property = gst_nmosaudioreceiver_set_property;
    object_class->get_property = gst_nmosaudioreceiver_get_property;

    add_property(object_class, 1, "node-id", "Node ID", "The ID of the node", "d49c85db-1c33-4f21-b160-58edd2af1810");
    add_property(
        object_class, 2, "node-config-file-location", "Node Config File Location",
        "The location of the configuration file for the node",
        "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/config/nmos_plugin_node_config_receiver.json");
    add_property(object_class, 3, "device-id", "Device ID", "The ID of the device",
                 "1ad20d7c-8c58-4c84-8f14-3cf3e3af164c");
    add_property(object_class, 4, "device-label", "Device Label", "The label to identify the device", "OSSRF Device");
    add_property(object_class, 5, "device-description", "Device Description", "Description of the device",
                 "OSSRF Device Description");
    add_property(object_class, 6, "receiver-id", "Receiver ID", "UUID of the receiver",
                 "db9f46cf-2414-4e25-b6c6-2078159857f9");
    add_property(object_class, 7, "receiver-label", "Label of the receiver", "Label of the receiver",
                 "BISECT OSSRF Audio Receiver");
    add_property(object_class, 8, "receiver-description", "Description of the receiver", "Description of the receiver",
                 "BISECT OSSRF Audio Receiver");
    add_property(object_class, 9, "destination-address", "Destination Address", "Address of the destination",
                 "127.0.0.1");

    gst_element_class_set_static_metadata(element_class, "NMOS Audio Receiver", "Source/Network",
                                          "Receives raw audio from NMOS", "Luis Ferreira <luis.ferreira@bisect.pt>");

    element_class->change_state = gst_nmosaudioreceiver_change_state;
}

// Object initialization
static void gst_nmosaudioreceiver_init(GstNmosaudioreceiver* self)
{
    GstPadTemplate* src_tmpl = gst_static_pad_template_get(&src_template);
    GstPad* ghost_src        = gst_ghost_pad_new_no_target_from_template("src", src_tmpl);
    gst_object_unref(src_tmpl);
    gst_element_add_pad(GST_ELEMENT(self), ghost_src);

    create_default_config_fields_audio_receiver(&self->config);
}

static gboolean plugin_init(GstPlugin* plugin)
{
    return gst_element_register(plugin, "nmosaudioreceiver", GST_RANK_NONE, GST_TYPE_NMOSAUDIORECEIVER);
}

#define VERSION "0.1"
#define PACKAGE "gst-nmos-audio-receiver-plugin"
#define PACKAGE_NAME "AMWA NMOS Sender and Receiver Framework Plugins"
#define GST_PACKAGE_ORIGIN "https://www.amwa.tv/"
#define GST_PACKAGE_LICENSE "Apache-2.0"

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, nmosaudioreceiver,
                  "Plugin to receive audio stream from NMOS application", plugin_init, VERSION, GST_PACKAGE_LICENSE,
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
