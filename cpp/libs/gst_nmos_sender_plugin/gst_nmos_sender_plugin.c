
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
} GstNmossender;

typedef struct _GstNmossenderClass
{
    GstBinClass parent_class;
} GstNmossenderClass;

G_DEFINE_TYPE_WITH_CODE(GstNmossender, gst_nmossender, GST_TYPE_BIN,
                        GST_DEBUG_CATEGORY_INIT(gst_nmossender_debug_category, "nmossender", 0, "NMOS Sender Plugin"));

/* Pad template */
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_REQUEST,
                            GST_STATIC_CAPS("video/x-raw, "
                                            "format=(string){ I420, NV12, RGB }, "
                                            "width=(int)[ 16, 4096 ], "
                                            "height=(int)[ 16, 2160 ], "
                                            "framerate=(fraction)[ 0/1, 120/1 ]"));

/* Class initialization */
static void gst_nmossender_class_init(GstNmossenderClass* klass)
{
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_template));

    gst_element_class_set_static_metadata(element_class, "NMOS Sender", "Sink/Network",
                                          "Processes raw video and sends it over RTP and UDP to NMOS client",
                                          "Luís Ferreira <luis.ferreira@bisect.pt>");
}

/* Object initialization */
static void gst_nmossender_init(GstNmossender* self)
{
    /* Create internal elements */
    self->payloader = gst_element_factory_make("rtpvrawpay", "payloader");
    self->udpsink   = gst_element_factory_make("udpsink", "udpsink");

    if(!self->payloader || !self->udpsink)
    {
        GST_ERROR_OBJECT(self, "Failed to create internal elements: payloader or udpsink");
        return;
    }

    /* Configure udpsink properties */
    g_object_set(G_OBJECT(self->udpsink), "host", "127.0.0.1", "port", 9999, NULL);

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