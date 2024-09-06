#!/bin/bash
set -eu
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export DEBIAN_FRONTEND=noninteractive
apt-get install -y sudo --option=Dpkg::Options::=--force-confdef

source ${SCRIPT_DIR}/../common/env.sh

check_env_var SSHD_PORT

apt-get update && apt-get install openssh-server

( 
echo "LogLevel DEBUG2"; 
echo "PermitRootLogin no"; 
echo "X11Forwarding yes"; 
echo "X11UseLocalhost no"; 
echo "Port ${SSHD_PORT}"; 
echo "PasswordAuthentication yes"; 
echo "Subsystem sftp /usr/lib/openssh/sftp-server"; 
) > /etc/ssh/sshd_config_bisect 

mkdir /run/sshd
