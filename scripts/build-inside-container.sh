#!/bin/bash

set -eu

SCRIPT_DIR="$(realpath "$(dirname "$0")")"
PROJECT_DIR=${SCRIPT_DIR}/..

docker compose -f ${PROJECT_DIR}/images/docker-compose-x86-development.yml build 
docker compose -f ${PROJECT_DIR}/images/docker-compose-x86-development.yml up -d

CONTAINER_NAME="images-ossrf-dev-1"

until docker exec -it $CONTAINER_NAME true > /dev/null 2>&1; do
  echo "Waiting for container $CONTAINER_NAME to start..."
  sleep 2
done

docker exec -it $CONTAINER_NAME bash -c "
  git clone https://github.com/AMWA-TV/nmos-sender-receiver-framework.git
  cd nmos-sender-receiver-framework
  conan profile detect
  ./scripts/setup.sh
  ./scripts/build.sh
"

docker exec -it $CONTAINER_NAME fish -C "cd /home/nmos/nmos-sender-receiver-framework"
