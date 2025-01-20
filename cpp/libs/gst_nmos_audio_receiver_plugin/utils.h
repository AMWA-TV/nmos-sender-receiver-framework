#pragma once
#include <string>
#include <glib.h>
#include <variant>
#include <gst/gst.h>
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

template <typename T = GstElement> class GstElementHandle
{
  public:
    static std::variant<GstElementHandle, std::nullptr_t> create_element(const char* factory_name,
                                                                         const char* element_name = nullptr)
    {
        T* elem = reinterpret_cast<T*>(gst_element_factory_make(factory_name, element_name));
        if(!elem)
        {
            return nullptr;
        }
        return GstElementHandle(elem);
    }

    static std::variant<GstElementHandle, std::nullptr_t> create_bin(const char* bin_name)
    {
        T* bin = reinterpret_cast<T*>(gst_bin_new(bin_name));
        if(!bin)
        {
            return nullptr;
        }
        return GstElementHandle(bin);
    }

    GstElementHandle(const GstElementHandle&)            = delete;
    GstElementHandle& operator=(const GstElementHandle&) = delete;

    // Move constructor
    GstElementHandle(GstElementHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

    // Move assignment
    GstElementHandle& operator=(GstElementHandle&& other) noexcept
    {
        if(this != &other)
        {
            if(handle_ != nullptr)
            {
                gst_object_unref(handle_);
            }
            handle_       = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~GstElementHandle()
    {
        if(handle_)
        {
            gst_object_unref(handle_);
        }
    }

    void reset(T* new_ptr = nullptr) { handle_ = new_ptr; }

    T* get() const { return handle_; }

    explicit operator bool() const { return (handle_ != nullptr); }

  private:
    explicit GstElementHandle(T* handle) : handle_(handle) {}

    T* handle_ = nullptr;
};

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
