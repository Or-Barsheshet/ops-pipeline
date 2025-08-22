#!/bin/bash
# Fail fast
set -euo pipefail

# --- Colors ---
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[BUILD]${NC} $1"; }
print_error()  { echo -e "${RED}[ERROR]${NC} $1"; }

print_status "Starting the build process..."

# Output dir
mkdir -p output

CFLAGS="-Wall -Wextra -Werror=implicit-function-declaration -O2"


if [[ "${FAST_TYPEWRITER:-0}" == "1" ]]; then
  CFLAGS="$CFLAGS -DTYPEWRITER_DELAY_US=0"
fi

SH_LDFLAGS="-Wl,-z,defs"

# ----- Build analyzer  -----
print_status "Building main application: analyzer"
gcc $CFLAGS -o output/analyzer main.c -ldl -lpthread || {
  print_error "Failed to build main application."
  exit 1
}

# ----- Build plugins -----
PLUGINS="logger typewriter uppercaser rotator flipper expander"

for plugin_name in $PLUGINS; do
  print_status "Building plugin: $plugin_name"
  gcc -fPIC -shared $CFLAGS $SH_LDFLAGS -o output/${plugin_name}.so \
      plugins/${plugin_name}.c \
      plugins/plugin_common.c \
      plugins/sync/consumer_producer.c \
      plugins/sync/monitor.c \
      -lpthread || {
    print_error "Failed to build plugin: $plugin_name"
    exit 1
  }
done

print_status "Build process completed successfully!"
