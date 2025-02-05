#pragma once
#include <string>
#include <glib.h>

struct node_fields_t
{
    std::string id;
    std::string configuration_location;
};

struct device_fields_t
{
    std::string id;
    std::string label;
    std::string description;
};

struct video_media_fields_t
{
    gint width;
    gint height;
    gint frame_rate_num;
    gint frame_rate_den;
    std::string sampling;
    std::string structure;
};

struct audio_media_fields_t
{
    std::string format;
    gint number_of_channels;
    gint sampling_rate;
    gint packet_time;
};

struct network_fields_t
{
    std::string source_address;
    std::string interface_name;
    std::string destination_address;
    gint destination_port;
};

struct config_fields_t
{
    node_fields_t node;
    device_fields_t device;
    gboolean is_audio;
    video_media_fields_t video_media_fields;
    audio_media_fields_t audio_sender_fields;
    std::string id;
    std::string label;
    std::string description;
    network_fields_t network;
    std::string interface_name;
    std::string address;
};
