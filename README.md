# ossrf

## Overview

Developing OSSRF for AWMA by Bisect.

## Platforms

Currently, only Linux is supported. 

## Requirements

Conan >= 2.0

CMake >= 3.16

## Code

Clone the repository:

`git clone git@github.com:bisect-pt/ossrf.git`

or

`git clone https://github.com/bisect-pt/ossrf.git`

## Setup Docker Containers

This will create the docker containers base on the docker compose:

    docker compose -f images/docker-compose-x86-development.yml build 

One of the containers is the ossrf-dev where you can find the development container.
The other is nmos-registry where will launch the NVIDIA NMOS Commissioning Controller

## Run Docker Containers

This will run the docker containers:

    docker compose -f images/docker-compose-x86-development.yml up -d

## Access the Development Container

### Using VSCode

Install `ms-vscode-remote.remote-ssh` extension on vscode and enter on the container.

### Using SSH

First you need to check you IP address. You can do it by running:

    hostname -i

Once you know your ip address you enter the container by doing:

    ssh -p 55555 bisect@{your-ip-address} -XY

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


