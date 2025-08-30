#!/bin/bash
set -euo pipefail

# colors for better readable output 
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'
print_status() { echo -e "${GREEN}[BUILD]${NC} $1"; }
print_error()  { echo -e "${RED}[ERROR]${NC} $1"; }

print_status "Starting build..."

# output dir
mkdir -p output

# flags
CFLAGS="-Wall -Wextra -O2 -fPIC"
LDFLAGS="-ldl -lpthread"
SH_LDFLAGS="-shared"

# build main analyzer
print_status "Building main analyzer"
gcc $CFLAGS -o output/analyzer \
  main.c \
  -ldl -lpthread

# build plugins
PLUGINS="logger uppercaser expander flipper rotator typewriter"

for plugin in $PLUGINS; do
  print_status "Building plugin: $plugin"
  gcc $CFLAGS -fPIC $SH_LDFLAGS -o output/${plugin}.so \
    plugins/${plugin}.c \
    plugins/plugin_common.c \
    plugins/sync/consumer_producer.c \
    plugins/sync/monitor.c \
    -lpthread -ldl
done

print_status "Build is done!"