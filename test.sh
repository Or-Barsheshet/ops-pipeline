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

# Test 3: Deadlock/Traffic Jam test
print_status "Running Test 3: Deadlock/Traffic Jam"
EXPECTED="[logger] B AC "
ACTUAL=$(echo -e "abc\n<END>" | $ANALYZER 50 uppercaser expander rotator rotator flipper logger typewriter | grep "\[logger\]" || true)
[ "$ACTUAL" == "$EXPECTED" ] || print_error "Test 3 FAILED (Expected '$EXPECTED', got '$ACTUAL')"
print_status "Test 3 PASSED"

# Test 4: Negative case - non-existent plugin
print_status "Running Test 4: Non-existent plugin"
# We expect the program to fail and print an error to stderr.
# The `! cmd` construct succeeds if cmd fails (returns non-zero).
if ! (echo "<END>" | $ANALYZER 10 non_existent_plugin 2>/dev/null); then
    print_status "Test 4 PASSED (Program correctly exited with an error)"
else
    print_error "Test 4 FAILED: Program should have failed for a non-existent plugin"
fi

# Test 5: Negative case - not enough arguments
print_status "Running Test 5: Not enough arguments"
if ! ($ANALYZER 10 2>/dev/null); then
    print_status "Test 5 PASSED (Program correctly exited with an error)"
else
    print_error "Test 5 FAILED: Program should have failed for not enough arguments"
fi


# Test 6: Typewriter prints verbatim and forwards downstream
print_status "Running Test 6: Typewriter (prints and forwards)"
EXPECTED_TYPE="[typewriter] HELLO"
EXPECTED_LOG="[logger] HELLO"
# שים לב: typewriter מדמה הדפסה עם usleep(100ms/char), אז הטסט הזה אורך ~0.5 שניות
ACTUAL_ALL=$(echo -e "hello\n<END>" | $ANALYZER 10 uppercaser typewriter logger)
ACTUAL_TYPE=$(echo "$ACTUAL_ALL" | grep -E "^\[typewriter\] " || true)
ACTUAL_LOG=$(echo "$ACTUAL_ALL" | grep -E "^\[logger\] " || true)
[ "$ACTUAL_TYPE" == "$EXPECTED_TYPE" ] || print_error "Test 6 FAILED (typewriter) Expected '$EXPECTED_TYPE', got '$ACTUAL_TYPE'"
[ "$ACTUAL_LOG" == "$EXPECTED_LOG" ] || print_error "Test 6 FAILED (logger downstream) Expected '$EXPECTED_LOG', got '$ACTUAL_LOG'"
print_status "Test 6 PASSED"

# Test 7: Typewriter alone (no downstream) still prints correctly
print_status "Running Test 7: Typewriter alone"
EXPECTED_TYPE_ALONE="[typewriter] Hi"
ACTUAL_TYPE_ALONE=$(echo -e "Hi\n<END>" | $ANALYZER 5 typewriter | grep -E "^\[typewriter\] " || true)
[ "$ACTUAL_TYPE_ALONE" == "$EXPECTED_TYPE_ALONE" ] || print_error "Test 7 FAILED (typewriter alone) Expected '$EXPECTED_TYPE_ALONE', got '$ACTUAL_TYPE_ALONE'"
print_status "Test 7 PASSED"

# Test 8: Typewriter prints each line and preserves order (with upstream transform)
print_status "Running Test 8: Typewriter multi-line order"
INPUT=$'a\nb\nc\n<END>'
EXPECTED_TYPES=$'[typewriter] A\n[typewriter] B\n[typewriter] C'
EXPECTED_LOGS=$'[logger] A\n[logger] B\n[logger] C'

OUT=$(echo -e "$INPUT" | $ANALYZER 8 uppercaser typewriter logger)
TYPES=$(echo "$OUT" | grep -E "^\[typewriter\] " || true)
LOGS=$(echo "$OUT"  | grep -E "^\[logger\] " || true)

[ "$TYPES" == "$EXPECTED_TYPES" ] || print_error "Test 8 FAILED (typewriter order)\nExpected:\n$EXPECTED_TYPES\nGot:\n$TYPES"
[ "$LOGS"  == "$EXPECTED_LOGS"  ] || print_error "Test 8 FAILED (logger order)\nExpected:\n$EXPECTED_LOGS\nGot:\n$LOGS"
print_status "Test 8 PASSED"



# Test 9: Backpressure — queue size 1 with multiple lines
print_status "Running Test 9: Typewriter backpressure (queue=1)"
INPUT=$'one\ntwo\nthree\n<END>'
EXPECTED_TYPES=$'[typewriter] ONE\n[typewriter] TWO\n[typewriter] THREE'
EXPECTED_LOGS=$'[logger] ONE\n[logger] TWO\n[logger] THREE'

OUT=$(echo -e "$INPUT" | $ANALYZER 1 uppercaser typewriter logger)
TYPES=$(echo "$OUT" | grep -E "^\[typewriter\] " || true)
LOGS=$(echo "$OUT"  | grep -E "^\[logger\] " || true)

[ "$TYPES" == "$EXPECTED_TYPES" ] || print_error "Test 9 FAILED (typewriter under backpressure)"
[ "$LOGS"  == "$EXPECTED_LOGS"  ] || print_error "Test 9 FAILED (logger under backpressure)"
print_status "Test 9 PASSED"



# Test 10: Typewriter mid-chain prints already-transformed text
print_status "Running Test 10: Typewriter mid-chain text correctness"
# uppercaser -> expander -> typewriter -> rotator(right) -> logger
INP=$'ab\n<END>'
OUT=$(echo -e "$INP" | $ANALYZER 10 uppercaser expander typewriter rotator logger)

TYPE_EXPECT='[typewriter] A B'
LOG_EXPECT='[logger] BA '  # rotator right on "A B" => "BA "

TYPE_ACTUAL=$(echo "$OUT" | grep -E "^\[typewriter\] " || true)
LOG_ACTUAL=$(echo "$OUT"  | grep -E "^\[logger\] " || true)

[ "$TYPE_ACTUAL" == "$TYPE_EXPECT" ] || print_error "Test 10 FAILED (typewriter mid-chain). Expected '$TYPE_EXPECT', got '$TYPE_ACTUAL'"
[ "$LOG_ACTUAL"  == "$LOG_EXPECT"  ] || print_error "Test 10 FAILED (logger after typewriter). Expected '$LOG_EXPECT', got '$LOG_ACTUAL'"
print_status "Test 10 PASSED"


print_status "Running Test 11: EOF without <END> closes cleanly"
OUT=$( (printf "x";) | $ANALYZER 4 uppercaser logger )
echo "$OUT" | grep -q "^\[logger\] X$" || print_error "Test 11 FAILED"
print_status "Test 11 PASSED"


# Test 12: Stress 1000 lines (queue=2)
print_status "Running Test 12: Stress 1000 lines (queue=2)"
OUT=$((yes "ab" | head -n 1000; echo "<END>") \
  | $ANALYZER 2 uppercaser logger \
  | grep -c "^\[logger\] ")
[ "$OUT" -eq 1000 ] || print_error "Test 12 FAILED (expected 1000 logger lines, got $OUT)"
print_status "Test 12 PASSED"


print_status() {
  echo -e "\033[0;32m[STATUS] $1\033[0m"
}

print_error() {
  echo -e "\033[0;31m[ERROR] $1\033[0m" >&2
  exit 1
}

# Define the analyzer executable if not already defined
ANALYZER=${ANALYZER:-./analyzer}

# ==============================================================================
# Test 13: Gauntlet — with added debug prints
# ==============================================================================

print_status "Running Test 13: Gauntlet — all plugins end-to-end (DEBUG MODE)"

# --- 1. Define Input ---
echo "[DEBUG] Defining GAUNTLET_INPUT..." >&2
GAUNTLET_INPUT=$'\n'"x"$'\n'"ab"$'\n<END>'
# Using cat with a heredoc to safely print multi-line input
echo "[DEBUG] Input is:" >&2
cat <<EOF >&2
---
$GAUNTLET_INPUT
---
EOF

# --- 2. Build Step ---
echo "[DEBUG] Starting build with FAST_TYPEWRITER=1..." >&2
FAST_TYPEWRITER=1 ./build.sh >/dev/null 2>&1
BUILD_RC=$?
echo "[DEBUG] Build finished with exit code: $BUILD_RC" >&2
if [ $BUILD_RC -ne 0 ]; then
    print_error "Test 13 FAILED: Build script failed with code $BUILD_RC"
fi

# --- 3. Define Pipeline ---
echo "[DEBUG] Assembling the pipeline command..." >&2
CMD=("$ANALYZER" 2 uppercaser rotator flipper expander typewriter rotator logger)
echo "[DEBUG] Pipeline command is: ${CMD[*]}" >&2


# --- 4. Execute Pipeline ---
echo "" >&2
echo "[DEBUG] >>> EXECUTING PIPELINE. This is the most likely place for a hang. <<<" >&2
echo "[DEBUG] Input is being piped to the command now..." >&2
echo "" >&2

OUT=""
RC=0

# Run with timeout if available to prevent infinite hang
if command -v timeout >/dev/null 2>&1; then
  echo "[DEBUG] Running with a 10s timeout." >&2
  OUT=$(printf "%b" "$GAUNTLET_INPUT" | timeout 10s "${CMD[@]}")
  RC=$?
else
  echo "[DEBUG] 'timeout' command not found. Running without a timeout." >&2
  OUT=$(printf "%b" "$GAUNTLET_INPUT" | "${CMD[@]}")
  RC=$?
fi

echo "" >&2
echo "[DEBUG] >>> PIPELINE EXECUTION FINISHED. <<<" >&2
echo "[DEBUG] Exit code (RC) from analyzer was: $RC" >&2
echo "" >&2

# --- 5. Analyze Exit Code ---
if [ $RC -ne 0 ]; then
    if [ $RC -eq 124 ]; then
      print_error "Test 13 FAILED: analyzer timed out after 10 seconds (possible deadlock)"
    else
      print_error "Test 13 FAILED: analyzer exited with non-zero code $RC"
    fi
fi

# Print the captured output for manual inspection
echo "[DEBUG] Captured output (\$OUT) from the pipeline is:" >&2
cat <<EOF >&2
---
$OUT
---
EOF

# --- 6. Start Assertions ---
echo "[DEBUG] Starting assertions on the captured output..." >&2

echo "[DEBUG] Checking line counts..." >&2
LOGGER_COUNT=$(printf "%s\n" "$OUT" | grep -cE "^\[logger\] " || true)
TYPEWRITER_COUNT=$(printf "%s\n" "$OUT" | grep -cE "^\[typewriter\] " || true)
echo "[DEBUG] Found $LOGGER_COUNT logger lines and $TYPEWRITER_COUNT typewriter lines." >&2

[ "$LOGGER_COUNT" -eq 3 ] || { printf "%s\n" "$OUT"; print_error "Test 13 FAILED: expected 3 logger lines, got $LOGGER_COUNT"; }
[ "$TYPEWRITER_COUNT" -eq 3 ] || { printf "%s\n" "$OUT"; print_error "Test 13 FAILED: expected 3 typewriter lines, got $TYPEWRITER_COUNT"; }

echo "[DEBUG] Checking exact content of logger output..." >&2
printf "%s\n" "$OUT" | grep -qx '\[logger\] '    || { printf "%s\n" "$OUT"; print_error "Test 13 FAILED: missing exact '[logger] ' (from empty input)"; }
echo "[DEBUG] Found '[logger] ' ✔" >&2
printf "%s\n" "$OUT" | grep -qx '\[logger\] X'   || { printf "%s\n" "$OUT"; print_error "Test 13 FAILED: missing exact '[logger] X'"; }
echo "[DEBUG] Found '[logger] X' ✔" >&2
printf "%s\n" "$OUT" | grep -qx '\[logger\] BA ' || { printf "%s\n" "$OUT"; print_error "Test 13 FAILED: missing exact '[logger] BA '"; }
echo "[DEBUG] Found '[logger] BA ' ✔" >&2

echo "[DEBUG] Checking the very last line of output..." >&2
LAST_LINE=$(printf "%s\n" "$OUT" | tail -n 1)
echo "[DEBUG] Last line is: '$LAST_LINE'" >&2
[ "$LAST_LINE" = "Pipeline shutdown complete" ] || { printf "%s\n" "$OUT"; print_error "Test 13 FAILED: last line should be 'Pipeline shutdown complete', got '$LAST_LINE'"; }
echo "[DEBUG] Last line is correct. ✔" >&2

echo ""
print_status "Test 13 PASSED"




echo -e "\n${GREEN} ALL TESTS PASSED SUCCESSFULLY! ${NC}"
exit 0