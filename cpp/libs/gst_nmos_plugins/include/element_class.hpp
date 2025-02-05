#pragma once
#include <string>
#include <glib.h>
#include <variant>
#include <gst/gst.h>
#include <nlohmann/json.hpp>

template <typename T = GstElement> class GstElementHandle
{
  public:
    static std::variant<GstElementHandle, std::nullptr_t> create_element(const char* factory_name,
                                                                         const char* element_name = nullptr)
    {
        T* elem = reinterpret_cast<T*>(gst_element_factory_make(factory_name, element_name));
        if(elem == nullptr)
        {
            return nullptr;
        }
        return GstElementHandle(elem);
    }

    static std::variant<GstElementHandle, std::nullptr_t> create_bin(const char* bin_name)
    {
        T* bin = reinterpret_cast<T*>(gst_bin_new(bin_name));
        if(bin == nullptr)
        {
            return nullptr;
        }
        return GstElementHandle(bin);
    }

    GstElementHandle(const GstElementHandle&)            = delete;
    GstElementHandle& operator=(const GstElementHandle&) = delete;

    GstElementHandle(GstElementHandle&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

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

    /// Destruction of this object will no longer unref the owned element.
    void forget(T* new_ptr = nullptr) { handle_ = new_ptr; }

    T* get() const { return handle_; }

  private:
    explicit GstElementHandle(T* handle) : handle_(handle) {}

    T* handle_ = nullptr;
};
