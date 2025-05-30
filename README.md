# AMWA Open Source Sender Receiver Framework

## Platforms

Currently, only Linux is supported.

## Requirements

Conan >= 2.0

CMake >= 3.16

## Code

Clone the repository:

`git clone git@github.com:AMWA-TV/nmos-sender-receiver-framework.git`

or

`git clone https://github.com/AMWA-TV/nmos-sender-receiver-framework.git`

## Setup Docker Containers

This will create the docker containers base on the docker compose:

    docker compose -f images/docker-compose-x86-development.yml build

One of the containers is the ossrf-dev where you can find the development container.
The other is nmos-registry where will launch the NVIDIA NMOS Commissioning Controller

## Run Docker Containers

This will run the docker containers:

    docker compose -f images/docker-compose-x86-development.yml up -d

## Access the Development Container

This project includes a development container to facilitate code development. When the container is built for the first time, it is empty, and the repository needs to be cloned again inside the container to build the project.

### Using VSCode

Install `ms-vscode-remote.remote-ssh` extension on vscode and enter on the container.

### Using SSH

First you need to check you IP address. You can do it by running:

    hostname -i

Once you know your ip address you enter the container by doing:

    ssh -p 55555 nmos@{your-ip-address} -XY

The password for this user is:

    nmos

## Access NVIDIA NMOS Commissioning Controller Container

### Access the NVIDIA NMOS Commissioning Controller UI

You can access the UI by opening your favorite browser and go to this link:

    http://localhost:8010/admin/

## Build

### Prepare Conan

If you have not used Conan before:

- create a directory:

  `mkdir ~/.conan2`

- confirm that the Conan version is suitable

  conan --version

- set the default Conan profile, e.g.

  conan profile detect --force

- You can map an outside of the container .conan2 folder in order to have persistent data storage. You just need to add the line below on to the volumes inside the docker-compose-x86-development.yml.

  - ~/.conan2:/home/nmos/.conan2:rw

### Install the dependencies using Conan

This only has to be done at the first time or after any of the dependencies change:

    ./scripts/setup.sh

### Build using CMake

    ./scripts/build.sh

### Demo ossrf-nmos-api

This example showcases the creation of one video/raw receiver and two video/raw senders, both on the NMOS and GStreamer sides. The receiver can be connected to either sender, allowing you to observe the different outputs.
While it is possible to create NMOS audio resources, GStreamer support for audio is not yet implemented.

#### Configuration file

Open `cpp/demos/ossrf-nmos-api/config/nmos_config.json` and adjust the following parameters:

- `host_addresses`
- `registry_address`
- `system_address`
- `interface_address`
- `system_address`

  This must be the address of the primary data interface.

  ***

#### To run:

`./build/Debug/cpp/demos/ossrf-nmos-api/ossrf-nmos-api -f ./cpp/demos/ossrf-nmos-api/config/nmos_config.json`

## Build Container and Code simultaneously

    ./scripts/build-inside-container.sh

## GStreamer Plugins

We have also developed GStreamer plugins (nmossender, nmosvideoreceiver, and nmosaudioreceiver) that integrate NMOS registration and control with the sending and receiving of raw ST2110 audio/video streams.

For detailed instructions on building, installing, and using these plugins (including examples of gst-launch-1.0 pipelines), please see the [Plugins Guide](/cpp/libs/gst_nmos_plugins/).
