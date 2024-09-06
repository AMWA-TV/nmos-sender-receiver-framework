#!/bin/bash
set -eu

export DEBIAN_FRONTEND=noninteractive
apt-get install -y sudo --option=Dpkg::Options::=--force-confdef

adduser --disabled-password --gecos '' bisect
adduser bisect sudo
echo 'bisect:bisect' | chpasswd
echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# access gst video plugins as non-root
usermod -aG video bisect
usermod -aG audio bisect
