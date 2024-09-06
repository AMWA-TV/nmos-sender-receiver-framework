#!/bin/bash
set -eu

#install clang
sudo apt-get update && sudo apt-get install -y clang-format

#check clang format
last_commit=$(git log -1 --pretty=format:"%H")
git show --name-only --pretty=format: $last_commit | grep -E '\.(cpp|hpp|c|h)$' | xargs -r clang-format -style=file -output-replacements-xml | grep "<replacement " && exit 1 || exit 0
