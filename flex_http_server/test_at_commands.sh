#!/bin/bash
# test_at_commands.sh - FLEX AT Command Testing Script
# ===================================================
# This script tests AT command communication with flex-fsk-tx devices

set -e

# Configuration (can be overridden by environment variables)
FLEX_DEVICE="${FLEX_DEVICE:-/dev/ttyUSB0}"
FLEX_BAUDRATE="${FLEX_BAUDRATE:-115200}"
TEST_FREQUENCY="${TEST_FREQUENCY:-916.0000}"
TEST_POWER="${TEST_POWER:-2}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-5}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage
show_usage() {
    cat << EOF
FLEX AT Command Testing Script
=============================

Tests AT command communication with flex-fsk-tx devices.

USAGE:
  $0 [OPTIONS]

OPTIONS:
  -d, --device DEVICE      Serial device path (default: $FLEX_DEVICE)
  -b, --baudrate RATE      Serial baudrate (default: $FLEX_BAUDRATE)
  -f, --frequency FREQ     Test frequency in MHz (default: $TEST_FREQUENCY)
  -p, --power LEVEL        Test power level 2-20 (default: $TEST_POWER)
  -t, --timeout SECONDS    Command timeout (default: $TIMEOUT_SECONDS)
  -h, --help              Show this help message

EXAMPLES:
  $0                                           # Test with defaults
  $0 -d /dev/ttyACM0 -b 9600                  # Different device/baudrate
  $0 -f 868.0000 -p 10                        # Different frequency/power
  $0 --device /dev/cu.SLAB_USBtoUART          # macOS device

ENVIRONMENT VARIABLES:
  FLEX_DEVICE=/dev/ttyUSB0     # Override default device
  FLEX_BAUDRATE=115200         # Override default baudrate
  TEST_FREQUENCY=916.0000      # Override test frequency
  TEST_POWER=2                 # Override test power
  TIMEOUT_SECONDS=5            # Override command timeout

REQUIREMENTS:
  - Device with flex-fsk-tx firmware installed
  - User must have read/write permissions to serial device
  - stty command available (part of coreutils)

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--device)
            FLEX_DEVICE="$2"
            shift 2
            ;;
        -b|--baudrate)
            FLEX_BAUDRATE="$2"
            shift 2
            ;;
        -f|--frequency)
            TEST_FREQUENCY="$2"
            shift 2
            ;;
        -p|--power)
            TEST_POWER="$2"
            shift 2
            ;;
        -t|--timeout)
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information."
            exit 1
            ;;
    esac
done

# Validate inputs
if [[ ! -c "$FLEX_DEVICE" ]]; then
    print_error "Device not found: $FLEX_DEVICE"
    print_status "Available devices:"
    ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | sed 's/^/  /' || echo "  (no USB/ACM devices found)"
    exit 1
fi

if [[ ! -r "$FLEX_DEVICE" || ! -w "$FLEX_DEVICE" ]]; then
    print_error "Insufficient permissions for $FLEX_DEVICE"
    print_status "Fix with one of these commands:"
    echo "  sudo chmod 666 $FLEX_DEVICE"
    echo "  sudo usermod -a -G dialout \$USER && newgrp dialout"
    exit 1
fi

# Validate numeric inputs
if ! [[ "$FLEX_BAUDRATE" =~ ^[0-9]+$ ]]; then
    print_error "Invalid baudrate: $FLEX_BAUDRATE"
    exit 1
fi

if ! [[ "$TEST_POWER" =~ ^[0-9]+$ ]] || (( TEST_POWER < 2 || TEST_POWER > 20 )); then
    print_error "Invalid power level: $TEST_POWER (must be 2-20)"
    exit 1
fi

if ! [[ "$TIMEOUT_SECONDS" =~ ^[0-9]+$ ]]; then
    print_error "Invalid timeout: $TIMEOUT_SECONDS"
    exit 1
fi

# Function to send AT command and wait for response
send_at_command() {
    local command="$1"
    local expected_response="$2"
    local timeout="$3"

    print_status "Sending: $command"

    # Configure serial port
    stty -F "$FLEX_DEVICE" "$FLEX_BAUDRATE" cs8 -cstopb -parenb raw -echo

    # Send command with timeout
    (
        echo -e "${command}\r"
        sleep "$timeout"
    ) > "$FLEX_DEVICE" &

    # Read response with timeout
    response=$(timeout "$timeout" cat "$FLEX_DEVICE" 2>/dev/null | tr -d '\r\n' | head -1 || echo "TIMEOUT")

    print_status "Received: $response"

    if [[ "$response" == "$expected_response" ]]; then
        print_success "Command successful"
        return 0
    elif [[ "$response" == "TIMEOUT" ]]; then
        print_error "Command timeout"
        return 1
    else
        print_error "Unexpected response (expected: $expected_response)"
        return 1
    fi
}

# Function to test device connectivity
test_device_connectivity() {
    print_status "Testing device connectivity..."

    # Check if device exists and is accessible
    if [[ -c "$FLEX_DEVICE" ]]; then
        print_success "Device found: $FLEX_DEVICE"
    else
        print_error "Device not found: $FLEX_DEVICE"
        return 1
    fi

    # Check permissions
    if [[ -r "$FLEX_DEVICE" && -w "$FLEX_DEVICE" ]]; then
        print_success "Device permissions OK"
    else
        print_warning "Device permissions may be insufficient"
    fi

    return 0
}

# Function to run AT command tests
run_at_tests() {
    local test_count=0
    local success_count=0

    print_status "Starting AT command tests..."
    echo

    # Test 1: Basic AT command
    print_status "Test 1/4: Basic AT communication"
    ((test_count++))
    if send_at_command "AT" "OK" "$TIMEOUT_SECONDS"; then
        ((success_count++))
    fi
    echo

    # Test 2: Set frequency
    print_status "Test 2/4: Set frequency to ${TEST_FREQUENCY} MHz"
    ((test_count++))
    if send_at_command "AT+FREQ=${TEST_FREQUENCY}" "OK" "$TIMEOUT_SECONDS"; then
        ((success_count++))
    fi
    echo

    # Test 3: Set power
    print_status "Test 3/4: Set power to ${TEST_POWER}"
    ((test_count++))
    if send_at_command "AT+POWER=${TEST_POWER}" "OK" "$TIMEOUT_SECONDS"; then
        ((success_count++))
    fi
    echo

    # Test 4: Prepare for data (but don't send data)
    print_status "Test 4/4: Prepare for data transmission"
    ((test_count++))
    if send_at_command "AT+SEND=10" "+SEND: READY" "$((TIMEOUT_SECONDS * 2))"; then
        ((success_count++))
        print_status "Canceling data transmission (test only)"
        # Send newline to cancel the data transmission
        echo -e "\r" > "$FLEX_DEVICE" 2>/dev/null || true
    fi
    echo

    # Summary
    print_status "Test Results: $success_count/$test_count tests passed"

    if (( success_count == test_count )); then
        print_success "All tests PASSED! Device is working correctly."
        return 0
    else
        print_warning "Some tests FAILED. Check device firmware and configuration."
        return 1
    fi
}

# Main execution
main() {
    echo "======================================"
    echo "FLEX AT Command Testing Script"
    echo "======================================"
    echo

    print_status "Configuration:"
    echo "  Device: $FLEX_DEVICE"
    echo "  Baudrate: $FLEX_BAUDRATE"
    echo "  Test Frequency: $TEST_FREQUENCY MHz"
    echo "  Test Power: $TEST_POWER"
    echo "  Timeout: $TIMEOUT_SECONDS seconds"
    echo

    # Test device connectivity
    if ! test_device_connectivity; then
        print_error "Device connectivity test failed"
        exit 1
    fi
    echo

    # Run AT command tests
    if run_at_tests; then
        echo
        print_success "FLEX device is ready for use!"
        print_status "You can now start the flex_http_server:"
        echo "  ./flex_http_server --verbose"
        exit 0
    else
        echo
        print_error "FLEX device testing failed!"
        print_status "Troubleshooting steps:"
        echo "  1. Verify flex-fsk-tx firmware is installed"
        echo "  2. Check device path and permissions"
        echo "  3. Try different baudrate if needed"
        echo "  4. Reset device and try again"
        echo "  5. Test manually: screen $FLEX_DEVICE $FLEX_BAUDRATE"
        exit 1
    fi
}

# Trap signals for cleanup
trap 'print_warning "Script interrupted"; exit 130' INT TERM

# Run main function
main "$@"
