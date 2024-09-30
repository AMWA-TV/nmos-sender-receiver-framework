#!/bin/bash
set -eu

HOME="/home/nmos"

if [ -d $HOME ] 
then
    echo "Directory $HOME exists." 
    sudo chown nmos.nmos $HOME
else
    echo "Directory $HOME does not exist."
    mkdir $HOME
fi

# start ssh service
sudo /usr/sbin/sshd -D -e -f /etc/ssh/sshd_config_nmos
