#pragma once
#include <string>
#include <glib.h>
#include <variant>
#include <gst/gst.h>
#include <nlohmann/json.hpp>
#include "../include/element_class.h"
#include "../include/nmos_configuration.h"

void create_default_config_fields_sender(config_fields_t* config);
void create_default_config_fields_video_receiver(config_fields_t* config);
void create_default_config_fields_audio_receiver(config_fields_t* config);

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

nlohmann::json create_video_sender_config(config_fields_t& config);
nlohmann::json create_audio_sender_config(config_fields_t& config);
