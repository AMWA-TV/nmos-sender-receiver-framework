#pragma once
#include <string>
#include <glib.h>
#include <nlohmann/json.hpp>

typedef struct node_fields_t
{
    gchar* id;
    gchar* configuration_location;
} node_fields_t;

typedef struct device_fields_t
{
    gchar* id;
    gchar* label;
    gchar* description;
} device_fields_t;

typedef struct media_fields_t
{
    gint width;
    gint height;
    gint frame_rate_num;
    gint frame_rate_density;
    gchar* sampling;
    gchar* structure;
} media_fields_t;

typedef struct network_fields_t
{
    gchar* source_address;
    gchar* interface_name;
    gchar* destination_address;
    gchar* destination_port;
} network_fields_t;

typedef struct config_fields_t
{
    node_fields_t node;
    device_fields_t device;
    media_fields_t media;
    gchar* sender_id;
    gchar* sender_label;
    gchar* sender_description;
    network_fields_t network;
} config_fields_t;

config_fields_t create_default_config_fields();

nlohmann::json create_node_config(config_fields_t& config);
nlohmann::json create_device_config(config_fields_t& config);
nlohmann::json create_sender_config(config_fields_t& config);

std::string get_node_id(char* node_configuration_location);

std::string get_node_config(char* node_configuration_location);

std::string get_device_id(char* device_configuration_location);

std::string get_device_config(char* device_configuration_location);

std::string get_sender_config(char* sender_configuration_location); 

