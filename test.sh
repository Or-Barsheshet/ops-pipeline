#!/bin/bash
set -e

# --- Colors and helpers ---
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'
print_status() { echo -e "\n${GREEN}[TEST]${NC} $1"; }
print_error() { echo -e "${RED}[TEST]${NC} $1"; exit 1; }

# 1. Build the project
print_status "Building the project..."
./build.sh

ANALYZER="./output/analyzer"

# --- Run Test Cases ---

# Test 1: Basic sanity check
print_status "Running Test 1: Sanity check (uppercaser -> logger)"
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n<END>" | $ANALYZER 10 uppercaser logger | grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 1 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 1 PASSED"

# Test 2: Empty string input
print_status "Running Test 2: Empty string input"
EXPECTED="[logger] "
ACTUAL=$(echo -e "\n<END>" | $ANALYZER 10 expander logger | grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 2 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 2 PASSED"

#Test 3: Non-existent plugin -> exit code 1
print_status "Running Test 3: Non-existent plugin should fail with exit code 1"
set +e
$ANALYZER 10 does_not_exist 1>out.tmp 2>err.tmp
STATUS=$?
set -e
[ $STATUS -eq 1 ] || print_error "Test 3 FAILED (Expected exit 1, got $STATUS)"
rm -f out.tmp err.tmp
print_status "Test 3 PASSED"

# Test 4: Valid unknown name (.so exists) -> should run successfully
print_status "Running Test 4: Unknown plugin name with existing .so should succeed"
# Prepare: copy an existing plugin .so (logger) to a new unfamiliar name
cp ./output/uppercaser.so ./output/alien.so || print_error "Test 4 FAILED (couldn't prepare alien.so)"
EXPECTED="[logger] HI"
ACTUAL=$(echo -e "hi\n<END>" | $ANALYZER 10 alien logger| grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || { rm -f ./output/alien.so; print_error "Test 4 FAILED (Expected '$EXPECTED', got '$ACTUAL')"; }
rm -f ./output/alien.so
print_status "Test 4 PASSED"

# Test 5: Duplicate plugin name in the same run -> exit code 1 + 'Multiple instances' message
print_status "Running Test 5: Duplicate plugin name should fail with exit code 1 and message"
set +e
$ANALYZER 10 logger logger 1>out.tmp 2>err.tmp
STATUS=$?
set -e
[ $STATUS -eq 1 ] || { cat err.tmp || true; rm -f out.tmp err.tmp; print_error "Test 5 FAILED (Expected exit 1, got $STATUS)"; }
if ! grep -q "Multiple instances" err.tmp; then
  cat err.tmp || true
  rm -f out.tmp err.tmp
  print_error "Test 5 FAILED (Missing 'Multiple instances' message in stderr)"
fi
rm -f out.tmp err.tmp
print_status "Test 5 PASSED"

echo -e "\n${GREEN}[TEST] All tests PASSED ✔${NC}"