# NMOS GStreamer Plugins

This repository contains GStreamer plugins that integrate AMWA NMOS functionality into your pipelines.  
Specifically, it provides an **NMOS Audio Receiver** (`nmosaudioreceiver`), **NMOS Video Receiver** (`nmosvideoreceiver`), and **NMOS Sender** (`nmossender`) that can dynamically that can receive and send ST2110 audio/video streams configured by the NMOS control panel.

## Table of Contents

## Table of Contents

1. [Overview](#overview)
2. [Building from Source](#building-from-source)
3. [Installing the Plugin](#installing-the-plugin)
   - [Installing via Script](#option-1-installing-via-script)
   - [Installing Manually](#option-2-installing-manually)
   - [Environment Variable](#option-3-environment-variable)
   - [Verifying Installation](#verifying-the-installation)
4. [Using the Plugin](#using-the-plugin)
   - [Inspecting the Plugin](#inspecting-the-plugin)
   - [Example Pipelines](#example-pipelines)
   - [NMOS Interface](#nmos-interface)
5. [Notes and Troubleshooting](#notes-and-troubleshooting)
6. [License](#license)


---

## Overview

The **NMOS plugins** communicate with an NMOS registry (using the AMWA [IS-04](https://specs.amwa.tv/is-04/) and [IS-05](https://specs.amwa.tv/is-05/) APIs) to automatically register and control the NMOS resources that send/receive raw ST2110 video/audio streams.

## Building from Source

To build the plugin, follow the build steps in the [**README**](/) inside the root directory of this project. The binary files for the plugin will then be created in `/build/Release/plugins/`.

## Installing the Plugin

Once you've built the plugin, you need to install it so that GStreamer can detect and use it.

### Option 1: Installing via Script

To simplify installation, you can use the provided installation script. This script installs the NMOS GStreamer plugins system-wide.

Run the following command:
```bash
./scripts/install_custom_gstreamer_plugins.sh
```

If the script is in a different directory, provide the path to your compiled plugins:
```bash
./scripts/install_custom_gstreamer_plugins.sh /path/to/your/plugins
```
By default, the script assumes that the compiled plugins are located in:
```bash
../build/Release/plugins/
```
**Removing the NMOS Plugins**

If you want to remove the NMOS plugins, run:
```bash
./scripts/install_custom_gstreamer_plugins.sh -r
```
or
```bash
./scripts/install_custom_gstreamer_plugins.sh --remove
```
This will delete nmossender, nmosvideoreceiver, and nmosaudioreceiver from the system.

### Option 2: Installing Manually

If you prefer a manual installation, follow these steps:
1. **Locate Your Compiled Plugins**

After building the project, the compiled .so (Linux), .dylib (macOS), or .dll (Windows) files will be in:
```
/build/Release/plugins/
```

2. **Copy the Plugins to a GStreamer Plugin Directory**

Copy the plugins to the GStreamer plugins folder.
By default it should be ``/usr/lib/x86_64-linux-gnu/gstreamer-1.0`` for Linux.

### Option 3: Environment Variable

The last option for the plugin usage envolves setting environment variable `GST_PLUGIN_PATH` to the directory where the binary files are saved. This way for the current shell session GStreamer will also look for plugins in the specified path. Choosing this option will require the setting of the variable everytime a new session is created.

i.e.

```bash
export GST_PLUGIN_PATH=/home/gst-plugins/
```

### Verifying the installation

Check if GStreamer recognizes the plugins:
```bash
gst-inspect-1.0 | grep "nmos"
```
If the installation was successful, you should see output similar to:
```
nmossender:  nmossender: NMOS Sender
nmosvideoreceiver:  nmosvideoreceiver: NMOS Video Receiver
nmosaudioreceiver:  nmosaudioreceiver: NMOS Audio Receiver
```

## Using the Plugin

### Inspecting the Plugin

When using these plugins, you can inspect them along with all the properties you can change, capabilities, and info with the following command:

```bash
gst-inspect-1.0 nmos(sender/videoreceiver/audioreceiver)
```

Some of the more important properties to change are as follows:

| Property                     | Description                                               |
|------------------------------|-----------------------------------------------------------|
| `node-id`                    | NMOS Node ID (UUID)                                       |
| `node-config-file-location`  | Path to the node configuration JSON file (String)         |
| `device-id`                  | NMOS Device ID (UUID)                                     |
| `device-label`               | A label for the NMOS Device (String)                      |
| `device-description`         | A description of the NMOS Device (String)                 |
| `receiver-id`                | NMOS Receiver ID (UUID)                                   |
| `receiver-label`             | A label for the NMOS Receiver (String)                    |
| `receiver-description`       | A description of the NMOS Receiver (String)               |
| `destination-address`        | IP address for the outgoing/incoming RTP stream (String)  |

**Important Note:** Currently, the node fields for the NMOS interface connection aren't configurable by properties but instead by a JSON file. An example can be found at `/cpp/demos/config/`.

### Example Pipelines
-----
#### Sender (Audio):
```bash
gst-launch-1.0 audiotestsrc is-live=true wave=square ! audio/x-raw, format=S24BE, rate=48000, channels=2, layout=interleaved ! nmossender destination-address="192.168.1.1" destination-port=5004
```

#### Audio Receiver:
```bash
gst-launch-1.0 -v nmosaudioreceiver destination-address="192.168.1.1" receiver-id="9dd4cb3e-7d28-411d-9939-b8e439bd8c2a" ! queue ! audioconvert ! wavescope ! ximagesink sync=false
```
-----

#### Sender (Video):
```bash
gst-launch-1.0 videotestsrc is-live=true timestamp-offset=1 pattern=ball ! videoconvert ! "video/x-raw, format=UYVP, sampling=YCbCr-4:2:2, width=460, height=240, clock-rate=9000, framerate=50/1" ! nmossender destination-address="192.168.1.1" source-address="192.168.1.1" destination-port=9999
```
#### Video Receiver:
```bash
gst-launch-1.0 -v nmosvideoreceiver destination-address="192.168.1.1" receiver-id="9dd4cb3e-7d28-411d-9939-b8e439bd8c2a" ! queue ! videoconvert ! autovideosink sync=false
```
-----
### NMOS Interface

The NMOS Interface allows users to manage senders and receivers through a graphical user interface. It provides an overview of active streams, transport methods, and detailed configurations.

The NMOS page can be accessed through the following link ```http://localhost:8010/admin/```

- **Senders Management**: Displays active senders, transport type (e.g., RTP Multicast), and allows toggling activation.
- **Receivers Management**: Lists available receivers, their bindings, supported media formats, and current connections.
- **Detailed View**: Clicking on a node, device, sender or receiver provides detailed configuration options, including transport settings and associated flows.
- **Connection Setup**: Users can establish connections between senders and receivers through a simple UI inside the receiver page, where they can freely connect and disconnect them.

Note: When changing the parameters inside the connect tab, be sure to activate the activation mode ```activate_immediate``` if you want to change the connection dynamically.

## Notes and Troubleshooting

- **State Management:** The plugin automatically transitions to PLAYING when valid SDP data is received. If the receiver is disabled from the NMOS registry side, the pipeline tears down its internal elements.
- **Flush and Dynamic Reconfiguration:** If you dynamically change streams at runtime (e.g., the NMOS registry activates a new source), the plugin will remove old elements and construct new ones on the fly without restarting the entire pipeline.
- **Verbose Debug:** If you need to see more logs, you can enable GStreamer debug categories:

```bash
GST_DEBUG=nmosaudioreceiver:5 gst-launch-1.0 nmosaudioreceiver ...
```
 
### Dependencies

- GStreamer (1.18+ recommended)

## License

This project is licensed under the Apache 2.0 License.
Please see the LICENSE file on the root directory for more details.
