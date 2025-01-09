
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
#include <gst/gst.h>
#include <gst/gstpad.h>

using namespace bisect;
using namespace bisect::sdp;

GST_DEBUG_CATEGORY_STATIC(gst_nmosreceiver_debug_category);
#define GST_CAT_DEFAULT gst_nmosreceiver_debug_category
#define GST_TYPE_NMOSRECEIVER (gst_nmosreceiver_get_type())
#define GST_NMOSRECEIVER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NMOSRECEIVER, GstNmosreceiver))
#define GST_NMOSRECEIVER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NMOSRECEIVER, GstNmosreceiverClass))
#define GST_IS_NMOSRECEIVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NMOSRECEIVER))
#define GST_IS_NMOSRECEIVER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NMOSRECEIVER))

typedef struct _GstNmosreceiver
{
    GstBin parent;
    GstPad* element_pad;
    GstPad* bin_pad;
    GstClock* clock;
    GstElement* bin;
    GstElement* udp_src;
    GstElement* rtp_video_depay;
    GstElement* rtp_jitter_buffer;
    GstElement* queue1;
    GstElement* video_rate;
    GstElement* identity;
    ossrf::nmos_client_uptr client;
    config_fields_t config;
    sdp_settings_t sdp_settings;
    bool nmos_active;
} GstNmosreceiver;

typedef struct _GstNmosreceiverClass
{
    GstBinClass parent_class;
} GstNmosreceiverClass;

G_DEFINE_TYPE_WITH_CODE(GstNmosreceiver, gst_nmosreceiver, GST_TYPE_BIN,
                        GST_DEBUG_CATEGORY_INIT(gst_nmosreceiver_debug_category, "nmosvideoreceiver", 0,
                                                "NMOS Receiver Plugin"))
// Pad template
static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw, "
                                            "format=(string){ I420, NV12, RGB, YUV422P, UYVP }; "
                                            "application/x-rtp, "
                                            "media=(string)video, "
                                            "clock-rate=(int)90000"));

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
static void gst_nmosreceiver_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GstNmosreceiver* self = GST_NMOSRECEIVER(object);

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

static void gst_nmosreceiver_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GstNmosreceiver* self = GST_NMOSRECEIVER(object);

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

void remove_old_bin(GstNmosreceiver* self)
{
    if(self->element_pad != nullptr)
    {
        gst_element_send_event(GST_ELEMENT(self), gst_event_new_flush_start());
        gst_element_send_event(GST_ELEMENT(self), gst_event_new_flush_stop(FALSE));

        gulong block_id =
            gst_pad_add_probe(self->element_pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, block_pad_probe_cb, NULL, NULL);

        self->clock = gst_element_get_clock(GST_ELEMENT(self));

        gst_element_set_state(self->bin, GST_STATE_NULL);

        gst_bin_remove(GST_BIN(self), self->bin);

        if(block_id != 0)
        {
            gst_pad_remove_probe(self->element_pad, block_id);
        }
        gst_object_unref(self->bin);
    }
}

void construct_pipeline(GstNmosreceiver* self)
{
    auto format = self->sdp_settings.format;
    try
    {
        bisect::nmoscpp::video_sender_info_t video_info = std::get<bisect::nmoscpp::video_sender_info_t>(format);
        auto& fr                                        = video_info.exact_framerate;
        g_print("\n===== SDP Settings =====\n"
                "Video (from variant):\n"
                "  - height: %d\n"
                "  - width:  %d\n"
                "  - framerate: %ld/%ld\n"
                "  - chroma_sub_sampling: %s\n"
                "  - structure (interlace mode): %s\n"
                "  - depth: %d\n"
                "Primary leg:\n"
                "  - destination_ip:   %s\n"
                "  - destination_port: %d\n"
                "\n",
                video_info.height, video_info.width, fr.numerator(), fr.denominator(),
                video_info.chroma_sub_sampling.c_str(), video_info.structure.name.c_str(), video_info.depth,
                self->sdp_settings.primary.destination_ip.value().c_str(),
                self->sdp_settings.primary.destination_port.value());

        self->bin = nullptr;

        self->queue1            = nullptr;
        self->video_rate        = nullptr;
        self->udp_src           = nullptr;
        self->rtp_jitter_buffer = nullptr;
        self->rtp_video_depay   = nullptr;
        self->identity          = nullptr;

        GstElement* bin = gst_bin_new("dynamic-bin");
        if(bin == nullptr)
        {
            GST_ERROR_OBJECT(self, "Failed to create bin");
            return;
        }

        self->queue1            = gst_element_factory_make("queue", nullptr);
        self->video_rate        = gst_element_factory_make("videorate", nullptr);
        self->udp_src           = gst_element_factory_make("udpsrc", nullptr);
        self->rtp_jitter_buffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
        self->rtp_video_depay   = gst_element_factory_make("rtpvrawdepay", nullptr);
        self->identity          = gst_element_factory_make("identity", nullptr);

        if(!self->udp_src || !self->rtp_jitter_buffer || !self->queue1 || !self->rtp_video_depay)
        {
            GST_ERROR_OBJECT(self, "Failed to create pipeline elements.");
            return;
        }

        g_object_set(G_OBJECT(self->udp_src), "address", self->sdp_settings.primary.destination_ip.value().c_str(),
                     "port", self->sdp_settings.primary.destination_port.value(), "buffer-size", 67108864, nullptr);

        // g_signal_connect(self->identity, "handoff", G_CALLBACK(identity_handoff_callback), NULL);

        GstCaps* caps = gst_caps_new_simple(
            "application/x-rtp", "media", G_TYPE_STRING, "video", "clock-rate", G_TYPE_INT, 90000, "encoding-name",
            G_TYPE_STRING, "RAW", "payload", G_TYPE_INT, 96, "width", G_TYPE_STRING,
            std::to_string(video_info.width).c_str(), "height", G_TYPE_STRING,
            std::to_string(video_info.height).c_str(), "format", G_TYPE_STRING, "UYVP", "sampling", G_TYPE_STRING,
            video_info.chroma_sub_sampling.c_str(), "depth", G_TYPE_STRING, std::to_string(video_info.depth).c_str(),
            "framerate", GST_TYPE_FRACTION, fr.numerator(), fr.denominator(), nullptr);

        g_object_set(G_OBJECT(self->udp_src), "caps", caps, nullptr);

        g_object_set(G_OBJECT(self->rtp_jitter_buffer), "do-retransmission", true, "do-lost", true, "mode", 2,
                     "latency", 0, nullptr);

        g_object_set(G_OBJECT(self->queue1), "max-size-buffers", 3, nullptr);

        gst_bin_add_many(GST_BIN(bin), self->udp_src, self->rtp_jitter_buffer, self->video_rate, self->queue1,
                         self->rtp_video_depay, self->identity, nullptr);

        gst_element_sync_state_with_parent(self->udp_src);
        gst_element_sync_state_with_parent(self->rtp_jitter_buffer);
        gst_element_sync_state_with_parent(self->queue1);
        gst_element_sync_state_with_parent(self->rtp_video_depay);
        gst_element_sync_state_with_parent(self->identity);

        self->bin = bin;

        if(gst_element_link_many(self->udp_src, self->rtp_jitter_buffer, self->queue1, self->rtp_video_depay,
                                 nullptr) == false)
        {
            GST_ERROR_OBJECT(self, "Failed to link elements inside the bin.");
            gst_object_unref(self->bin);
            return;
        }

        // Create a ghost pad from the depay's src
        GstPad* bin_src_pad = gst_element_get_static_pad(self->rtp_video_depay, "src");
        if(bin_src_pad == nullptr)
        {
            GST_ERROR_OBJECT(self, "Failed to get src pad from rtp_video_depay.");
            gst_object_unref(bin);
            return;
        }

        GstPad* bin_ghost_pad = gst_ghost_pad_new("src", bin_src_pad);
        gst_object_unref(bin_src_pad);
        self->bin_pad = bin_ghost_pad;

        if(bin_ghost_pad == nullptr || gst_element_add_pad(bin, bin_ghost_pad) == false)
        {
            GST_ERROR_OBJECT(self, "Failed to create or add ghost pad to bin.");
            gst_object_unref(bin);
            return;
        }

        gst_bin_add(GST_BIN(self), bin);

        GstPad* plugin_pad = gst_element_get_static_pad(GST_ELEMENT(self), "src");
        if(plugin_pad == nullptr)
        {
            GST_ERROR_OBJECT(self, "Failed to get plugin ghost pad.");
            gst_object_unref(bin);
            return;
        }

        self->element_pad = plugin_pad;

        if(gst_ghost_pad_set_target(GST_GHOST_PAD(self->element_pad), bin_ghost_pad) == false)
        {
            GST_ERROR_OBJECT(self, "Failed to link plugin ghost pad to bin's ghost pad.");
            gst_object_unref(bin);
            return;
        }

        gst_object_unref(plugin_pad);
    }
    catch(std::bad_variant_access const& ex)
    {
        g_print("Invalid format sent for current receiver.\n");
    }
}

void create_nmos(GstNmosreceiver* self)
{

    const auto node_config_json                           = create_node_config(self->config);
    const auto device_config_json                         = create_device_config(self->config);
    nlohmann::json_abi_v3_11_3::json receiver_config_json = nullptr;

    receiver_config_json = create_receiver_config(self->config);

    // g_print("Node Configuration: %s\n", node_config_json.dump().c_str());
    // g_print("Device Configuration: %s\n", device_config_json.dump().c_str());
    // g_print("Receiver Configuration: %s\n", receiver_config_json.dump().c_str());

    auto receiver_callback = [self](const std::optional<std::string>& sdp, bool master_enabled) {
        // Debug print
        fmt::print("Receiver Activation Callback: SDP={}, Master Enabled={}\n", sdp.has_value() ? sdp.value() : "None",
                   master_enabled);

        if(sdp)
        {
            fmt::print("Received SDP: {}\n", sdp.value());
            auto sdp_settings = parse_sdp(sdp.value());
            if(sdp_settings.has_value())
            {
                self->sdp_settings = sdp_settings.value();
                remove_old_bin(self);
                construct_pipeline(self);
                auto time = gst_element_get_base_time(GST_ELEMENT(self));
                gst_element_set_state(GST_ELEMENT(self), GST_STATE_PLAYING);
                gst_pad_set_offset(self->bin_pad, (long)(gst_clock_get_time(self->clock) - time));
            }
        }
        else
        {
            fmt::print("No SDP provided.\n");
        }
        fmt::print("Master enabled: {}\n", master_enabled);
    };

    auto result = ossrf::nmos_client_t::create(self->config.node.id, node_config_json.dump());
    if(result.has_value())
    {
        self->client = std::move(result.value());
        self->client->add_device(device_config_json.dump());
        self->client->add_receiver(self->config.device.id, receiver_config_json.dump(), receiver_callback);
        GST_INFO_OBJECT(self, "NMOS client initialized successfully.");
    }
    else
    {
        GST_ERROR_OBJECT(self, "Failed to initialize NMOS client.");
    }
}

static GstStateChangeReturn gst_nmosreceiver_change_state(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret;
    GstNmosreceiver* self = GST_NMOSRECEIVER(element);

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

    ret = GST_ELEMENT_CLASS(gst_nmosreceiver_parent_class)->change_state(element, transition);

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
static void gst_nmosreceiver_class_init(GstNmosreceiverClass* klass)
{
    GObjectClass* object_class     = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_template));

    object_class->set_property = gst_nmosreceiver_set_property;
    object_class->get_property = gst_nmosreceiver_get_property;

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
                 "BISECT OSSRF Video Receiver");
    add_property(object_class, 8, "receiver-description", "Description of the receiver", "Description of the receiver",
                 "BISECT OSSRF Video Receiver");
    add_property(object_class, 9, "destination-address", "Destination Address", "Address of the destination",
                 "127.0.0.1");

    gst_element_class_set_static_metadata(element_class, "NMOS Video Receiver", "Source/Network",
                                          "Receives raw video from NMOS", "Luis Ferreira <luis.ferreira@bisect.pt>");

    element_class->change_state = gst_nmosreceiver_change_state;
}

// Object initialization
static void gst_nmosreceiver_init(GstNmosreceiver* self)
{
    GstPadTemplate* src_tmpl = gst_static_pad_template_get(&src_template);
    GstPad* ghost_src        = gst_ghost_pad_new_no_target_from_template("src", src_tmpl);
    gst_object_unref(src_tmpl);
    gst_element_add_pad(GST_ELEMENT(self), ghost_src);

    create_default_config_fields(&self->config);
}

static gboolean plugin_init(GstPlugin* plugin)
{
    return gst_element_register(plugin, "nmosvideoreceiver", GST_RANK_NONE, GST_TYPE_NMOSRECEIVER);
}

#define VERSION "0.1"
#define PACKAGE "gst-nmos-receiver-plugin"
#define PACKAGE_NAME "AMWA NMOS Sender and Receiver Framework Plugins"
#define GST_PACKAGE_ORIGIN "https://www.amwa.tv/"
#define GST_PACKAGE_LICENSE "Apache-2.0"

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, nmosvideoreceiver,
                  "Plugin to receive video stream from NMOS application", plugin_init, VERSION, GST_PACKAGE_LICENSE,
                  PACKAGE_NAME, GST_PACKAGE_ORIGIN)
