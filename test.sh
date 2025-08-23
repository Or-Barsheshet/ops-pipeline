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

# Test 3: Single space through all plugins -> stays a single space
print_status "Running Test 3: Single space through all plugins"
EXPECTED="[logger]  "
ACTUAL=$(echo -e " \n<END>" | $ANALYZER 10 uppercaser rotator flipper expander typewriter logger | grep "^\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 3 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 3 PASSED"


#Test 4: Non-existent plugin -> exit code 1
print_status "Running Test 4: Non-existent plugin should fail with exit code 1"
set +e
$ANALYZER 10 does_not_exist 1>out.tmp 2>err.tmp
STATUS=$?
set -e
[ $STATUS -eq 1 ] || print_error "Test 4 FAILED (Expected exit 1, got $STATUS)"
rm -f out.tmp err.tmp
print_status "Test 4 PASSED"

# Test 5: Valid unknown name (.so exists) -> should run successfully
print_status "Running Test 5: Unknown plugin name with existing .so should succeed"
# Prepare: copy an existing plugin .so (logger) to a new unfamiliar name
cp ./output/uppercaser.so ./output/alien.so || print_error "Test 5 FAILED (couldn't prepare alien.so)"
EXPECTED="[logger] HI"
ACTUAL=$(echo -e "hi\n<END>" | $ANALYZER 10 alien logger| grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || { rm -f ./output/alien.so; print_error "Test 5 FAILED (Expected '$EXPECTED', got '$ACTUAL')"; }
rm -f ./output/alien.so
print_status "Test 5 PASSED"

# Test 6: Duplicate plugin name in the same run -> exit code 1 + 'Multiple instances' message
print_status "Running Test 6: Duplicate plugin name should fail with exit code 1 and message"
set +e
$ANALYZER 10 logger logger 1>out.tmp 2>err.tmp
STATUS=$?
set -e
[ $STATUS -eq 1 ] || { cat err.tmp || true; rm -f out.tmp err.tmp; print_error "Test 6 FAILED (Expected exit 1, got $STATUS)"; }
if ! grep -q "Multiple instances" err.tmp; then
  cat err.tmp || true
  rm -f out.tmp err.tmp
  print_error "Test 6 FAILED (Missing 'Multiple instances' message in stderr)"
fi
rm -f out.tmp err.tmp
print_status "Test 6 PASSED"

# Test 7: Uppercaser + Rotator -> Logger
print_status "Running Test 7: Uppercaser + Rotator"
EXPECTED="[logger] OHELL"
ACTUAL=$(echo -e "HELLO\n<END>" | $ANALYZER 10 rotator logger | grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 7 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 7 PASSED"

# Test 8: Flipper -> Logger
print_status "Running Test 8: Flipper"
EXPECTED="[logger] OLLEH"
ACTUAL=$(echo -e "HELLO\n<END>" | $ANALYZER 10 flipper logger | grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 8 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 8 PASSED"

# Test 9: Expander -> Logger
print_status "Running Test 9: Expander"
EXPECTED="[logger] H E L L O"
ACTUAL=$(echo -e "HELLO\n<END>" | $ANALYZER 10 expander logger | grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 9 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 9 PASSED"

# Test 10: Max-length line (1024 chars) survives -> length check at logger
print_status "Running Test 10: Max-length line (1024 chars) through uppercaser -> logger"
LONG=$(printf 'a%.0s' {1..1024})
# Grab the length of the [logger] line (without trailing newline)
ACTUAL_LEN=$(echo -e "$LONG\n<END>" | $ANALYZER 10 uppercaser logger \
  | awk '/^\[logger\]/{print length($0); exit}')
EXPECTED_LEN=$((9 + 1024))   # "[logger] " is 9 chars + 1024 A's
[ "$ACTUAL_LEN" -eq "$EXPECTED_LEN" ] || print_error "Test 10 FAILED (Expected length $EXPECTED_LEN, got $ACTUAL_LEN)"
print_status "Test 10 PASSED"

# Test 11: Backpressure & ordering with queue_size=1 (uppercaser -> typewriter)
print_status "Running Test 11: Backpressure & ordering with queue_size=1"
INPUT=$'one\ntwo\nthree\n<END>\n'
EXPECTED=$'[typewriter] ONE\n[typewriter] TWO\n[typewriter] THREE'
ACTUAL=$(printf "%s" "$INPUT" | $ANALYZER 1 uppercaser typewriter | grep "^\[typewriter\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 11 FAILED (Expected:\n$EXPECTED\nGot:\n$ACTUAL)"
print_status "Test 11 PASSED"

# Test 12: <END> in the middle stops pipeline; lines after <END> must be ignored
print_status "Running Test 12: <END> mid-stream should stop processing"
ACTUAL=$(printf "one\n<END>\ntwo\n" | $ANALYZER 10 logger | grep "^\[logger\]" || true)
EXPECTED="[logger] one"
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 12 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 12 PASSED"

# Test 13: Composition check (rotator -> expander -> flipper -> logger) on 'AB'
# rotator: 'AB'->'BA', expander: 'BA'->'B A', flipper: 'B A'->'A B'
print_status "Running Test 13: rotator -> expander -> flipper -> logger"
EXPECTED="[logger] A B"
ACTUAL=$(echo -e "AB\n<END>" | $ANALYZER 10 rotator expander flipper logger | grep "^\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 13 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 13 PASSED"

# Test 14: Invalid queue size (0) -> exit code 1 and Usage printed to STDOUT
print_status "Running Test 14: Invalid queue size should exit 1 and print usage"
set +e
$ANALYZER 0 logger 1>out.tmp 2>err.tmp
STATUS=$?
set -e
[ $STATUS -eq 1 ] || { cat err.tmp || true; rm -f out.tmp err.tmp; print_error "Test 14 FAILED (Expected exit 1, got $STATUS)"; }
if ! grep -q "^Usage: ./analyzer" out.tmp; then
  echo "STDOUT:"; cat out.tmp || true
  echo "STDERR:"; cat err.tmp || true
  rm -f out.tmp err.tmp
  print_error "Test 14 FAILED (Usage message not found on STDOUT)"
fi
rm -f out.tmp err.tmp
print_status "Test 14 PASSED"

# Test 15: Multiple <END> markers should stop processing after the first
print_status "Running Test 15: Multiple <END> markers"
ACTUAL=$(printf "first\n<END>\nsecond\n<END>\n" | $ANALYZER 10 logger | grep "^\[logger\]" || true)
EXPECTED="[logger] first"
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 15 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 15 PASSED"

# Test 16: Queue saturation with queue_size=1 and many inputs
print_status "Running Test 16: Queue saturation stress"
INPUT=$(printf "line%d\n" {1..20}; echo "<END>")
ACTUAL_COUNT=$(echo "$INPUT" | $ANALYZER 1 logger | grep "^\[logger\]" | wc -l)
EXPECTED_COUNT=20
[ "$ACTUAL_COUNT" -eq "$EXPECTED_COUNT" ] || print_error "Test 16 FAILED (Expected $EXPECTED_COUNT lines, got $ACTUAL_COUNT)"
print_status "Test 16 PASSED"

# Test 17: Duplicate plugin name (uppercaser twice) -> exit code 1 + 'Multiple instances' message
print_status "Running Test 17: Duplicate plugin name (uppercaser twice) should fail"
set +e
$ANALYZER 10 uppercaser uppercaser logger 1>out.tmp 2>err.tmp
STATUS=$?
set -e
[ $STATUS -eq 1 ] || { cat err.tmp || true; rm -f out.tmp err.tmp; print_error "Test 17 FAILED (Expected exit 1, got $STATUS)"; }
if ! grep -q "Multiple instances" err.tmp; then
  echo "STDERR:"; cat err.tmp || true
  rm -f out.tmp err.tmp
  print_error "Test 17 FAILED (Missing 'Multiple instances' message in stderr)"
fi
rm -f out.tmp err.tmp
print_status "Test 17 PASSED"

# Test 18: Complex composition expander->flipper->rotator->logger
print_status "Running Test 18: expander->flipper->rotator"
EXPECTED="[logger] AC B "
ACTUAL=$(echo -e "ABC\n<END>" | $ANALYZER 10 expander flipper rotator logger | grep "^\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 18 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 18 PASSED"

# Test 19: Pipeline without logger should not produce [logger] output
print_status "Running Test 19: No logger in pipeline"
ACTUAL=$(echo -e "hello\n<END>" | $ANALYZER 10 uppercaser typewriter | grep "^\[logger\]" || true)
[ -z "$ACTUAL" ] || print_error "Test 19 FAILED (Expected no [logger] output, got '$ACTUAL')"
print_status "Test 19 PASSED"


echo -e "\n${GREEN}[TEST] All tests PASSED ✔${NC}"