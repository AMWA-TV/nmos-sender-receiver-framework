#!/bin/bash

SCRIPT_DIR="$(realpath "$(dirname "$0")")"
PROJECT_DIR=${SCRIPT_DIR}/../..

for i in $(find ${PROJECT_DIR}/cpp -name '*.cpp' -o -name '*.h');
do
  if ! grep -q Copyright $i
  then
    cat ${SCRIPT_DIR}/copyright.txt $i >$i.new && mv $i.new $i
  fi
done