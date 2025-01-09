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

typedef struct config_fields_t
{
    node_fields_t node;
    device_fields_t device;
    std::string id;
    std::string label;
    std::string description;
    std::string address;
    gint port;
    std::string interface_name;
} config_fields_t;

void create_default_config_fields(config_fields_t* config);

nlohmann::json create_node_config(config_fields_t& config);
nlohmann::json create_device_config(config_fields_t& config);
nlohmann::json create_receiver_config(config_fields_t& config);

std::string translate_video_format(const std::string& gst_format);

std::string translate_audio_format(const std::string& gst_format);

std::string get_node_id(char* node_configuration_location);

std::string get_node_config(std::string node_configuration_location);

std::string get_device_id(char* device_configuration_location);

std::string get_device_config(char* device_configuration_location);

std::string get_sender_config(char* sender_configuration_location);
