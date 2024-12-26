#pragma once
#include <string>
#include <glib.h>
#include <nlohmann/json.hpp>

typedef struct node_fields_t
{
    std::string id;
    std::string configuration_location;
} node_fields_t;

typedef struct device_fields_t
{
    std::string id;
    std::string label;
    std::string description;
} device_fields_t;

typedef struct video_media_fields_t
{
    gint width;
    gint height;
    gint frame_rate_num;
    gint frame_rate_den;
    std::string sampling;
    std::string structure;
} video_media_fields_t;

typedef struct audio_media_fields_t
{
    std::string format;
    gint number_of_channels;
    gint sampling_rate;
    gint packet_time;
} audio_media_fields_t;

typedef struct network_fields_t
{
    std::string source_address;
    std::string interface_name;
    std::string destination_address;
    std::string destination_port;
} network_fields_t;

typedef struct config_fields_t
{
    node_fields_t node;
    device_fields_t device;
    gboolean is_audio;
    video_media_fields_t video_media_fields;
    audio_media_fields_t audio_sender_fields;
    std::string sender_id;
    std::string sender_label;
    std::string sender_description;
    network_fields_t network;
} config_fields_t;

void create_default_config_fields(config_fields_t* config);

nlohmann::json create_node_config(config_fields_t& config);
nlohmann::json create_device_config(config_fields_t& config);
nlohmann::json create_video_sender_config(config_fields_t& config);
nlohmann::json create_audio_sender_config(config_fields_t& config);

std::string translate_video_format(const std::string& gst_format);

std::string translate_audio_format(const std::string& gst_format);

std::string get_node_id(char* node_configuration_location);

std::string get_node_config(char* node_configuration_location);

std::string get_device_id(char* device_configuration_location);

std::string get_device_config(char* device_configuration_location);

std::string get_sender_config(char* sender_configuration_location);
