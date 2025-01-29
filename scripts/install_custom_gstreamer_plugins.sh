#!/bin/bash

set -e  # Exit on error

# Default system-wide plugin path for Linux
SYSTEM_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"

REMOVE_MODE=false
PLUGIN_SOURCE_DIR=""

# Parse arguments
for arg in "$@"; do
    case "$arg" in
        -r|--remove)
            REMOVE_MODE=true
            ;;
        *)
            PLUGIN_SOURCE_DIR="$arg"
            ;;
    esac
done

# If remove mode is enabled, do NOT check for the plugin directory
if $REMOVE_MODE; then
    echo "Removing NMOS-related plugins..."
    
    sudo -v  # Ask for password upfront

    # Find and remove NMOS-related plugins
    sudo rm -rf "$SYSTEM_PLUGIN_DIR/libgstnmossender.so"
    sudo rm -rf "$SYSTEM_PLUGIN_DIR/libgstnmosvideoreceiver.so"
    sudo rm -rf "$SYSTEM_PLUGIN_DIR/libgstnmosaudioreceiver.so"

    echo "NMOS plugins removed from: $SYSTEM_PLUGIN_DIR"

    # Force GStreamer to refresh the plugin registry
    echo "Refreshing GStreamer plugin cache..."
    sudo gst-inspect-1.0 --gst-disable-registry-fork >/dev/null 2>&1 || true

    echo "Verifying removal..."
    if gst-inspect-1.0 | grep "nmos"; then
        echo "⚠️ NMOS plugins still detected! Try restarting your system or manually clearing GStreamer cache."
    else
        echo "✅ NMOS plugins successfully removed."
    fi

    exit 0  # Exit after removing
fi

# If no path is given, set the default
if [ -z "$PLUGIN_SOURCE_DIR" ]; then
    PLUGIN_SOURCE_DIR="$(realpath "$(dirname "$0")/../build/Release/plugins")"
fi

if [ ! -d "$PLUGIN_SOURCE_DIR" ]; then
    echo "Error: Directory $PLUGIN_SOURCE_DIR does not exist!"
    exit 1
fi

echo "Installing plugins from: $PLUGIN_SOURCE_DIR"

echo "Requesting permission for system-wide installation..."
sudo -v  # Ask for password once

install_plugins() {
    echo "Installing custom GStreamer plugins to $SYSTEM_PLUGIN_DIR..."

    sudo mkdir -p "$SYSTEM_PLUGIN_DIR"
    sudo cp -v "$PLUGIN_SOURCE_DIR"/*.so "$SYSTEM_PLUGIN_DIR" 2>/dev/null || true
    sudo cp -v "$PLUGIN_SOURCE_DIR"/*.dll "$SYSTEM_PLUGIN_DIR" 2>/dev/null || true
    sudo cp -v "$PLUGIN_SOURCE_DIR"/*.dylib "$SYSTEM_PLUGIN_DIR" 2>/dev/null || true

    echo "Plugins installed in: $SYSTEM_PLUGIN_DIR"
    
    echo "Updating GStreamer plugin registry..."
    sudo gst-inspect-1.0 --gst-plugin-path="$SYSTEM_PLUGIN_DIR" >/dev/null 2>&1
}

case "$(uname -s)" in
    Linux*)
        install_plugins
        ;;
    *)
        echo "Unsupported OS."
        exit 1
        ;;
esac

echo "Verifying installation..."
if gst-inspect-1.0 | grep "nmos"; then
    echo "✅ NMOS plugins successfully installed."
else
    echo "⚠️ NMOS plugins not found. Check $SYSTEM_PLUGIN_DIR."
fi
