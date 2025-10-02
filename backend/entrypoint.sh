#!/bin/bash
set -e
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j --config Debug
exec "$@"