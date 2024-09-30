#!/bin/bash
set -eu

export DEBIAN_FRONTEND=noninteractive
apt-get install -y sudo --option=Dpkg::Options::=--force-confdef

adduser --disabled-password --gecos '' nmos
adduser nmos sudo
echo 'nmos:nmos' | chpasswd
echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# access gst video plugins as non-root
usermod -aG video nmos
usermod -aG audio nmos
