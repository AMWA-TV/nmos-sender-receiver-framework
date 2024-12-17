
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

#include "ossrf_c_nmos_api.h"
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
    GstElement* payloader;
    GstElement* udpsink;
    gchar* source_address;
    gchar* interface_name;
    gchar* destination_address;
    gchar* destination_port;
    GstCaps* caps;
    gint width;
    gint height;
    const gchar* format;
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
                                            "format=(string){ I420, NV12, RGB } "));

/* Set properties so element variables can change depending on them */
static void gst_nmossender_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    GstNmossender* self = GST_NMOSSENDER(object);

    switch(property_id)
    {
    case 1:
        g_free(self->source_address);
        self->source_address = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "host", self->source_address, NULL);
        break;

    case 2:
        g_free(self->interface_name);
        self->interface_name = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "sync", TRUE, "bind-address", self->interface_name, NULL);
        break;

    case 3:
        g_free(self->destination_address);
        self->destination_address = g_value_dup_string(value);
        g_object_set(G_OBJECT(self->udpsink), "host", self->destination_address, NULL);
        break;

    case 4:
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

/* Not needed for now:
Auxiliary function to validate if caps are compatible */
// static gboolean gst_nmossender_validate_caps(GstCaps* caps)
// {
//     gboolean valid = FALSE;

//     for(guint i = 0; i < gst_caps_get_size(caps); i++)
//     {
//         GstStructure* structure = gst_caps_get_structure(caps, i);

//         const gchar* media_type = gst_structure_get_name(structure);
//         if(g_strcmp0(media_type, "video/x-raw") == 0)
//         {
//             valid = TRUE;
//             break;
//         }
//     }

//     return valid;
// }

/* Event handler for the sink pad */
static gboolean gst_nmossender_sink_event(GstPad* pad, GstObject* parent, GstEvent* event)
{
    GstNmossender* self = GST_NMOSSENDER(parent);

    switch(GST_EVENT_TYPE(event))
    {
    case GST_EVENT_CAPS: {
        GstCaps* caps = NULL;
        gst_event_parse_caps(event, &caps);

        if(caps)
        {
            gchar* caps_str = gst_caps_to_string(caps);
            g_print("\n\nReceived CAPS from upstream: %s\n", caps_str);

            for(guint i = 0; i < gst_caps_get_size(caps); i++)
            {
                const GstStructure* structure = gst_caps_get_structure(caps, i);
                // Convert the structure to a string
                gchar* structure_str = gst_structure_to_string(structure);
                GST_INFO_OBJECT(self, "Caps Structure %u: %s", i, structure_str);
                g_free(structure_str);

                // If you want to retrieve individual fields (e.g., width, height, format)
                gint width, height;
                const gchar* format;
                if(gst_structure_get_int(structure, "width", &width))
                {
                    self->width = width;
                }
                if(gst_structure_get_int(structure, "height", &height))
                {
                    self->height = height;
                }
                if((format = gst_structure_get_string(structure, "format")))
                {
                    self->format = format;
                }
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

    // g_print("Width: %d, Height: %d, Format: %s\n", self->width, self->height, self->format);
    /* Pass the event downstream */
    return gst_pad_event_default(pad, parent, event);
}

/* State Change so it doesn't boot NMOS without pipeline being set to playing */
static GstStateChangeReturn gst_nmossender_change_state(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret;
    GstNmossender* self = GST_NMOSSENDER(element);

    switch(transition)
    {
    case GST_STATE_CHANGE_NULL_TO_READY: {
        // FIX ME: Example of using the nmos_client_create function, need to
        // change it later so it isnt hardcoded and is instead a property
        const char* config_location =
            "/home/nmos/repos/nmos-sender-receiver-framework/cpp/demos/ossrf-nmos-api/config/nmos_plugin_config.json";

        GST_INFO_OBJECT(self, "Initializing NMOS client...");

        // FIX ME: change inner workings of plugin
        nmos_client_t* client = nmos_client_create(config_location);
        if(client)
        {
            nmos_client_add_device(client, config_location);
            nmos_client_add_sender(client);
            // nmos_client_add_receiver(client);
            // nmos_client_remove_sender(client);
            // nmos_client_remove_receiver(client);
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
                                                        "Port of the destination (default: 9999)", "9999",
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata(element_class, "NMOS Sender", "Sink/Network",
                                          "Processes raw video and sends it over RTP and UDP to NMOS client",
                                          "Luis Ferreira <luis.ferreira@bisect.pt>");

    element_class->change_state = gst_nmossender_change_state;
}

/* Object initialization */
static void gst_nmossender_init(GstNmossender* self)
{
    self->payloader = gst_element_factory_make("rtpvrawpay", "payloader");
    self->udpsink   = gst_element_factory_make("udpsink", "udpsink");

    if(!self->payloader || !self->udpsink)
    {
        GST_ERROR_OBJECT(self, "Failed to create internal elements: payloader or udpsink");
        return;
    }

    // default udpsink properties
    g_object_set(G_OBJECT(self->udpsink), "host", "127.0.0.1", "port", 9999, NULL);

    // Add elements to the bin
    gst_bin_add_many(GST_BIN(self), self->payloader, self->udpsink, NULL);
    gst_element_link(self->payloader, self->udpsink);

    // create and configure the sink pad
    GstPad* payloader_sinkpad = gst_element_get_static_pad(self->payloader, "sink");
    GstPad* sink_ghost_pad    = gst_ghost_pad_new("sink", payloader_sinkpad);

    if(!sink_ghost_pad)
    {
        GST_ERROR_OBJECT(self, "Failed to create ghost pad for sink");
        gst_object_unref(payloader_sinkpad);
        return;
    }

    // added ghost pad to the element to enable
    // connectivity between layers of element
    gst_pad_set_event_function(sink_ghost_pad, gst_nmossender_sink_event);

    gst_element_add_pad(GST_ELEMENT(self), sink_ghost_pad);

    gst_object_unref(payloader_sinkpad);
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