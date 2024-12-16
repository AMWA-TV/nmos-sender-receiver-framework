
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

#include <gst/gst.h>
#include <gst/gstpad.h>
#include <ossrf_c_nmos_api.h>

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
    GstElement* payloader;
    GstElement* udpsink;
    gchar* source_address;
    gchar* interface_name;
    gchar* destination_address;
    gchar* destination_port;
} GstNmossender;

typedef struct _GstNmossenderClass
{
    GstBinClass parent_class;
} GstNmossenderClass;

G_DEFINE_TYPE_WITH_CODE(GstNmossender, gst_nmossender, GST_TYPE_BIN,
                        GST_DEBUG_CATEGORY_INIT(gst_nmossender_debug_category, "nmossender", 0, "NMOS Sender Plugin"))

/* Pad template */
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_REQUEST,
                            GST_STATIC_CAPS("video/x-raw, "
                                            "format=(string){ I420, NV12, RGB }, "
                                            "width=(int)[ 16, 4096 ], "
                                            "height=(int)[ 16, 2160 ], "
                                            "framerate=(fraction)[ 0/1, 120/1 ]"));

static void gst_nmossender_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GstNmossender* self = GST_NMOSSENDER(object);

    switch(property_id)
    {
    case 1: // source-address property
        g_free(self->source_address);
        self->source_address = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "host", self->source_address, NULL);
        break;

    case 2: // interface-name property
        g_free(self->interface_name);
        self->interface_name = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "sync", TRUE, "bind-address", self->interface_name, NULL);
        break;

    case 3: // destination-address property
        g_free(self->destination_address);
        self->destination_address = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "host", self->destination_address, NULL);
        break;

    case 4: // destination-port property
        g_free(self->destination_port);
        self->destination_port = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "port", atoi(self->destination_port), NULL);
        break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
    }
}

static void gst_nmossender_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    GstNmossender* self = GST_NMOSSENDER(object);

    switch(property_id)
    {
    case 1: g_value_set_string(value, self->source_address); break;

    case 2: g_value_set_string(value, self->interface_name); break;

    case 3: g_value_set_string(value, self->destination_address); break;

    case 4: g_value_set_string(value, self->destination_port); break;

    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
    }
}

static GstStateChangeReturn gst_nmossender_change_state(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret;
    GstNmossender* self = GST_NMOSSENDER(element);

    switch(transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY: {
        // Example of using the nmos_client_create function
        const char* config_location =
            "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/ossrf-nmos-api/config/nmos_config.json";

        GST_INFO_OBJECT(self, "Initializing NMOS client...");
        // FIX ME: change inner workings of plugin
        nmos_client_t* client = nmos_client_create(config_location);
        if(client)
        {
            nmos_client_add_device(client, config_location);
            nmos_client_add_sender(client);
            nmos_client_add_receiver(client);
            nmos_client_remove_sender(client);
            nmos_client_remove_receiver(client);
            GST_INFO_OBJECT(self, "NMOS client initialized successfully.");
        }
        else
        {
            GST_ERROR_OBJECT(self, "Failed to initialize NMOS client.");
        }
    }
    break;

    default: break;
    }

    // This is needed to work ¯\_(ツ)_/¯
    ret = GST_ELEMENT_CLASS(gst_nmossender_parent_class)->change_state(element, transition);

    return ret;
}

/* Class initialization */
static void gst_nmossender_class_init(GstNmossenderClass* klass)
{
    GObjectClass* object_class     = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_template));

    object_class->set_property = gst_nmossender_set_property;
    object_class->get_property = gst_nmossender_get_property;

    g_object_class_install_property(G_OBJECT_CLASS(klass), 1,
                                    g_param_spec_string("source-address", "Source Address",
                                                        "The address of the source (default: 127.0.0.1)", "127.0.0.1",
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(G_OBJECT_CLASS(klass), 2,
                                    g_param_spec_string("interface-name", "Interface Name",
                                                        "Name of the interface (default: wlp1s0)", "wlp1s0",
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(G_OBJECT_CLASS(klass), 3,
                                    g_param_spec_string("destination-address", "Destination Address",
                                                        "The address of the destination (default: 127.0.0.1)",
                                                        "127.0.0.1", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(G_OBJECT_CLASS(klass), 4,
                                    g_param_spec_string("destination-port", "Destination Port",
                                                        "Port of the destination (default: 5004)", "5004",
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata(element_class, "NMOS Sender", "Sink/Network",
                                          "Processes raw video and sends it over RTP and UDP to NMOS client",
                                          "Luis Ferreira <luis.ferreira@bisect.pt>");

    element_class->change_state = gst_nmossender_change_state;
}

/* Object initialization */
static void gst_nmossender_init(GstNmossender* self)
{

    /* Set default property values */
    self->source_address      = g_strdup("127.0.0.1");
    self->interface_name      = g_strdup("wlp1s0");
    self->destination_address = g_strdup("127.0.0.1");
    self->destination_port    = g_strdup("5004");

    /* Create internal elements */
    self->payloader = gst_element_factory_make("rtpvrawpay", "payloader");
    self->udpsink   = gst_element_factory_make("udpsink", "udpsink");

    if(!self->payloader || !self->udpsink)
    {
        GST_ERROR_OBJECT(self, "Failed to create internal elements: payloader or udpsink");
        return;
    }

    /* Configure udpsink properties */
    g_object_set(G_OBJECT(self->udpsink), "host", self->destination_address, self->destination_port, 9999, NULL);

    /* Add elements to the bin and link them */
    gst_bin_add_many(GST_BIN(self), self->payloader, self->udpsink, NULL);

    if(!gst_element_link(self->payloader, self->udpsink))
    {
        GST_ERROR_OBJECT(self, "Failed to link payloader to udpsink");
        return;
    }

    /* Get static pads from internal elements */
    GstPad* payloader_sinkpad = gst_element_get_static_pad(self->payloader, "sink");
    if(!payloader_sinkpad)
    {
        GST_ERROR_OBJECT(self, "Failed to get static sink pad from payloader");
        return;
    }

    /* Create ghost pads and add them to the nmossender element */
    GstPad* sink_ghost_pad = gst_ghost_pad_new("sink", payloader_sinkpad);
    if(!sink_ghost_pad)
    {
        GST_ERROR_OBJECT(self, "Failed to create ghost pad for sink");
        gst_object_unref(payloader_sinkpad);
        return;
    }

    /* Add ghost pads to the nmossender element */
    gst_element_add_pad(GST_ELEMENT(self), sink_ghost_pad);

    /* Unref the original static pads since they are no longer needed */
    gst_object_unref(payloader_sinkpad);
}

static gboolean plugin_init(GstPlugin* plugin)
{
    return gst_element_register(plugin, "nmossender", GST_RANK_NONE, GST_TYPE_NMOSSENDER);
}

#define VERSION "0.1"
#define PACKAGE "gst-plugins-ugly"
#define PACKAGE_NAME "GStreamer Ugly Plugins (Ubuntu)"
#define GST_PACKAGE_ORIGIN "http://bisect.pt/"
#define GST_PACKAGE_LICENSE "LGPL"

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, nmossender, "Plugin to send to NMOS application video stream",
                  plugin_init, VERSION, GST_PACKAGE_LICENSE, PACKAGE_NAME, GST_PACKAGE_ORIGIN)