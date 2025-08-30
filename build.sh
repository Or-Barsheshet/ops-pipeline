#!/bin/bash
set -Eeuo pipefail

# coloring for readable output
grn='\033[0;32m'
red='\033[0;31m'
rst='\033[0m'
log_status()   { echo -e "${grn}[BUILD]${rst} $1"; }
log_fail() { echo -e "${red}[ERROR]${rst} $1"; }

log_status "begin build"

# output directory
mkdir -p output

# flags
cflags="-Wall -Wextra -O2 -fPIC"
shared_link="-shared"

# build analyzer
log_status "building analyzer"
gcc -o output/analyzer main.c $cflags -lpthread -ldl

# build plugins
plugins=(logger uppercaser expander flipper rotator typewriter)

for p in "${plugins[@]}"; do
  log_status "building plugin: $p"
  gcc -fPIC $shared_link $cflags -o "output/$p.so" \
    "plugins/$p.c" \
    plugins/plugin_common.c \
    plugins/sync/consumer_producer.c \
    plugins/sync/monitor.c \
    -lpthread -ldl
done

log_status "build complete"