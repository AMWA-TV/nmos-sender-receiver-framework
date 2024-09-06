#!/bin/bash

set -eu

docker build -t ossrf-dev . -f dev/Dockerfile
