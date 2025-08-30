#!/bin/bash
set -e

RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
BLUE="\033[0;34m"
NC="\033[0m" # No Color

print_status() {
    echo -e "${GREEN}[PASS]${NC} $1"
}
print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

PASS="${GREEN}Pass${NC}"
FAIL="${RED}Fail${NC}"


# Build the project before running tests
if ./build.sh; then
    echo -e "${GREEN}Building complete${NC}"
else
    echo -e "${RED}Build failed${NC}"
    exit 1
fi


# Test each plugin seperate
# Test Logger
EXPECTED="[logger] Hello its me"
ACTUAL=$(echo -e "Hello its me\n<END>" | ./output/analyzer 5 logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test logger plugin: $PASS"
else
    echo -e "Test logger plugin: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi


# Test Typewriter
EXPECTED="[typewriter] I was wondering"
ACTUAL=$(echo -e "I was wondering\n<END>" | ./output/analyzer 5 typewriter | grep "\[typewriter\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test typewriter plugin: $PASS"
else
    echo -e "Test typewriter plugin: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Test Uppercaser
EXPECTED="[logger] IF AFTER ALL THIS YEARS"
ACTUAL=$(echo -e "if after all this years\n<END>" | ./output/analyzer 5 uppercaser logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test uppercaser plugin: $PASS"
else
    echo -e "Test uppercaser plugin: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Test Rotator
EXPECTED="[logger] tyou would like to mee"
ACTUAL=$(echo -e "you would like to meet\n<END>" | ./output/analyzer 5 rotator logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test rotator plugin: $PASS"
else
    echo -e "Test rotator plugin: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Test Flipper
EXPECTED="[logger] teem ot ekil dluow uoy"
ACTUAL=$(echo -e "you would like to meet\n<END>" | ./output/analyzer 5 flipper logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test flipper plugin: $PASS"
else
    echo -e "Test flipper plugin: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Test Expander
EXPECTED="[logger] s o   h e l l o"
ACTUAL=$(echo -e "so hello\n<END>" | ./output/analyzer 5 expander logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test expander plugin: $PASS"
else
    echo -e "Test expander plugin: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Example for the instructions
EXPECTED="[logger] HELLO"
ACTUAL=$(echo "hello"$'\n<END>' | ./output/analyzer 10 uppercaser logger | grep "\[logger\]")

if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo -e "Example for the instructions: Test uppercaser + logger: PASS"
else
    echo -e "Example for the instructions: Test uppercaser + logger: FAIL (Expected '$EXPECTED', got '$ACTUAL')"
    exit 1
fi

# Test plugin in combination
# Test uppercaser expander rotator logger
EXPECTED="[logger] MF R O "
ACTUAL=$(echo -e "from\n<END>" | ./output/analyzer 5 uppercaser expander rotator logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test for plugins: uppercaser expander rotator logger: $PASS"
else
        echo -e "Test for plugins: uppercaser expander rotator logger: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
        exit 1
fi

# Test special characters uppercaser expander rotator logger
EXPECTED="[logger] '1 2 3 4 ! # # % % & & ' "
ACTUAL=$(echo -e "1234!##%%&&''\n<END>" | ./output/analyzer 3 uppercaser expander rotator logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test special chars for plugins: uppercaser expander rotator logger: $PASS"
else
    echo -e "Test special chars for plugins: uppercaser expander rotator logger: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Test all plugins together
EXPECTED="$(cat <<'EOF'
[logger] we are
[typewriter] WE R A   E 
Pipeline shutdown complete
EOF
)"
ACTUAL=$(echo -e "we are\n<END>" | ./output/analyzer 2 logger uppercaser expander flipper rotator typewriter 2>&1)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test all plugins together: $PASS"
else
    echo -e "Test all plugins together: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi


# Test all plugins together with special characters
EXPECTED="$(cat <<'EOF'
[typewriter] 1234!##%%&&''
[logger] 1' ' & & % % # # ! 4 3 2 
Pipeline shutdown complete
EOF
)"
ACTUAL=$(echo -e "1234!##%%&&''\n<END>" | ./output/analyzer 6 typewriter uppercaser flipper expander rotator logger)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test special chars for plugins: uppercaser expander rotator logger: $PASS"
else
    echo -e "Test special chars for plugins: uppercaser expander rotator logger: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# # Test all plugins together with single character input
EXPECTED="$(cat <<'EOF'
[typewriter] h
[logger] H
Pipeline shutdown complete
EOF
)"
ACTUAL=$(echo -e "h\n<END>" | ./output/analyzer 5 typewriter uppercaser flipper expander rotator logger)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test single character for all plugins: $PASS"
else
    echo -e "Test single character for all plugins: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# #Test longest input string 1024 characters
# INPUT=$(printf 'a%.0s' {1..1024})
# EXPECTED_OUTPUT=$(printf 'A%.0s' {1..1024})
# EXPECTED="[logger] ${EXPECTED_OUTPUT}"
# ACTUAL=$(echo -e "${INPUT}\n<END>" | ./output/analyzer 2 uppercaser logger | grep "\[logger\]")
# if [ "$ACTUAL" = "$EXPECTED" ]; then
#     echo -e "Test longest input possible 1024 characters: $PASS"
# else
#     exp_len=${#EXPECTED}
#     act_len=${#ACTUAL}
#     echo -e "Test longest input possible 1024 characters: $FAIL (Expected length $exp_len, Got $act_len)"
#     exit 1
# fi

# Test longest input string 1024 characters
INPUT=$(printf 'a%.0s' {1..1024})
EXPECTED_OUTPUT=$(printf 'A%.0s' {1..1024})
EXPECTED="[logger] ${EXPECTED_OUTPUT}"
ACTUAL=$(
  { printf '%s\n' "$INPUT"; printf '<END>'; } \
  | ./output/analyzer 2 uppercaser logger \
  | grep -m1 "\[logger\]"
)
if [ "$ACTUAL" = "$EXPECTED" ]; then
  echo -e "Test longest input possible 1024 characters: $PASS"
else
  exp_len=${#EXPECTED}
  act_len=${#ACTUAL}
  echo -e "Test longest input possible 1024 characters: $FAIL (Expected length $exp_len, Got $act_len)"
  exit 1
fi




# Test plugins together with 3 spaces input
EXPECTED="[typewriter]      "
ACTUAL=$(echo -e "   \n<END>" | ./output/analyzer 5 uppercaser rotator expander typewriter | grep "\[typewriter\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test space input plugins: $PASS"
else
    echo -e "Test space input plugins: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# Test empty input
EXPECTED="[logger] "
ACTUAL=$(echo -e "\n<END>" | ./output/analyzer 5 uppercaser rotator expander logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test empty input plugins: $PASS"
else
    echo -e "Test empty input plugins: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send only <END> to analyzer
EXPECTED="Pipeline shutdown complete"
ACTUAL=$(echo -e "<END>" | ./output/analyzer 5 typewriter uppercaser rotator expander logger)
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test only <END> input: $PASS"
else
    echo -e "Test only <END> input: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send input with multiple <END> to analyzer
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n<END>\n<END>" | ./output/analyzer 5 uppercaser logger | grep "\[logger\]")
if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test multiple <END>: $PASS"
else
    echo -e "Test multiple <END>: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send input with multiple and more input than queue capacity to analyzer
EXPECTED=$(echo -e "[logger] HELLO\n[logger] ITS\n[logger] ME\n[logger] OMER\n[logger] BYE")
ACTUAL=$(echo -e "hello\nits\nme\nOmer\nbye\n<END>\nhello again\n<END>" \
  | ./output/analyzer 2 uppercaser logger \
  | grep "\[logger\]")

if [ "$ACTUAL" = "$EXPECTED" ]; then
    echo -e "Test multiple and more input than queue capacity: $PASS"
else
    echo -e "Test multiple and more input than queue capacity: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

strip_ansi() { sed -E 's/\x1B\[[0-9;]*[mK]//g'; }

# send negative queue capacity to analyzer
EXPECTED="[ERROR] Queue size must be a positive integer."
ACTUAL=$(./output/analyzer -5 uppercaser logger </dev/null 2>&1 | strip_ansi)
if [[ "$ACTUAL" == *"$EXPECTED"* ]]; then
    echo -e "Test negative queue capacity: $PASS"
else
    echo -e "Test negative queue capacity: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send zero queue capacity to analyzer
EXPECTED="[ERROR] Queue size must be a positive integer."
ACTUAL=$(./output/analyzer 0 uppercaser logger </dev/null 2>&1 | strip_ansi)
if [[ "$ACTUAL" == *"$EXPECTED"* ]]; then
    echo -e "Test zero queue capacity: $PASS"
else
    echo -e "Test zero queue capacity: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send non-integer queue capacity to analyzer
EXPECTED="[ERROR] Queue size must be a positive integer."
ACTUAL=$(./output/analyzer hi uppercaser logger </dev/null 2>&1 | strip_ansi)
if [[ "$ACTUAL" == *"$EXPECTED"* ]]; then
    echo -e "Test non-integer queue capacity: $PASS"
else
    echo -e "Test non-integer queue capacity: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send no plugins to analyzer
EXPECTED="[ERROR] Not enough arguments."
ACTUAL=$(./output/analyzer 10 </dev/null 2>&1 | strip_ansi)
if [[ "$ACTUAL" == *"$EXPECTED"* ]]; then
    echo -e "Test no plugins: $PASS"
else
    echo -e "Test no plugins: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# send invalid plugin name to analyzer
EXPECTED="[ERROR] dlopen failed for 'output/strangerdanger.so': output/strangerdanger.so: cannot open shared object file: No such file or directory"
ACTUAL=$(./output/analyzer 10 uppercaser logger strangerdanger </dev/null 2>&1 | strip_ansi)
if [[ "$ACTUAL" == *"$EXPECTED"* ]]; then
    echo -e "Test invalid plugin name: $PASS"
else
    echo -e "Test invalid plugin name: $FAIL (Expected '$EXPECTED'; Got '$ACTUAL')"
    exit 1
fi

# stress test with 30 input queue size 2
#Passed

echo -e "${GREEN}Passed all tests.${NC}"
exit 0


