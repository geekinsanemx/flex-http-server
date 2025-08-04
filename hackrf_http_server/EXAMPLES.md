# HackRF HTTP/TCP Server - Usage Examples

This document provides comprehensive examples and use cases for the HackRF HTTP/TCP Server.

## Quick Start

### 1. Basic Setup
```bash
# Start server with default configuration
./hackrf_http_server

# Start with verbose logging to see detailed processing
./hackrf_http_server --verbose

# Start in debug mode (no transmission, creates IQ file)
./hackrf_http_server --debug --verbose
```

### 2. Send Your First Message
```bash
# HTTP API (modern, recommended) - BOTH capcode and message are REQUIRED
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Hello World"}'

# TCP Serial (legacy)
echo '1122334|Hello World|931937500' | nc localhost 16175
```

## HTTP JSON API Examples

### Basic Messages
```bash
# Required fields only (uses DEFAULT_FREQUENCY from config)
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Test message"}'

# Complete message with all parameters
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1122334,
    "message": "Complete message with frequency",
    "frequency": 925516000
  }'

# Different capcode formats
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 37137, "message": "SHORT capcode format"}'

curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 123456789, "message": "LONG capcode format"}'
```

### Real-World Scenarios
```bash
# Emergency alert
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 911911,
    "message": "EMERGENCY: Fire alarm activated in Building A",
    "frequency": 931937500
  }'

# System notification
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 12345,
    "message": "Server maintenance scheduled for 2AM-4AM tonight"
  }'

# Status update with timestamp
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 99999,
    "message": "All systems operational - '$(date)'"
  }'

# Different priority levels
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1000001, "message": "LOW: Routine maintenance reminder"}'

curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 2000002, "message": "MEDIUM: Database backup starting"}'

curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 3000003, "message": "HIGH: Security breach detected"}'
```

### Validation Testing (Error Cases)
```bash
# Test missing capcode (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"message": "Missing capcode field"}'

# Expected response: {"error":"Missing required field: capcode must be specified","code":400}

# Test missing message (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334}'

# Expected response: {"error":"Missing required field: message must be specified","code":400}

# Test empty message (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": ""}'

# Test zero capcode (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 0, "message": "Zero capcode"}'

# Test authentication failure (should return 401)
curl -v -X POST http://localhost:16180/ \
  -u admin:wrongpassword \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "This will fail"}'

# Test invalid JSON (should return 400)
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "missing quote}'

# Test wrong method (should return 405)
curl -v -X GET http://localhost:16180/ \
  -u admin:passw0rd
```

### Optional Frequency Examples
```bash
# Use default frequency (frequency field omitted)
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Using DEFAULT_FREQUENCY"}'

# Specify custom frequency
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1122334,
    "message": "Custom frequency message",
    "frequency": 925516000
  }'

# Zero frequency (should use DEFAULT_FREQUENCY)
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1122334,
    "message": "Zero frequency defaults to config",
    "frequency": 0
  }'
```

## TCP Serial Protocol Examples

### Basic Usage
```bash
# Format: CAPCODE|MESSAGE|FREQUENCY
echo '12345|Hello from TCP|931937500' | nc localhost 16175

# Message with timeout to prevent hanging
echo '99999|Status update|925516000' | timeout 10 nc localhost 16175

# Multiple messages
{
  echo '11111|First message|931937500'
  echo '22222|Second message|931937500'
  echo '33333|Third message|931937500'
} | nc localhost 16175
```

### Scripted Usage
```bash
#!/bin/bash
# send_alert.sh - Send alert via TCP protocol

CAPCODE=${1:-1122334}  # Default capcode if not provided
MESSAGE=${2:-"Default alert message"}
FREQUENCY=${3:-931937500}  # Default frequency

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 CAPCODE MESSAGE [FREQUENCY]"
    echo "Example: $0 1122334 'System alert' 925516000"
    exit 1
fi

echo "${CAPCODE}|${MESSAGE}|${FREQUENCY}" | nc localhost 16175
if [ $? -eq 0 ]; then
    echo "Alert sent successfully"
else
    echo "Failed to send alert"
    exit 1
fi
```

### TCP Usage Examples
```bash
# Call the script
./send_alert.sh 1122334 "Emergency alert"
./send_alert.sh 911911 "Fire alarm" 931937500
./send_alert.sh 99999 "System status: OK"
```

## Authentication Management

### Setting Up Users
```bash
# Create new passwords file with first user
htpasswd -c passwords admin

# Add additional users with bcrypt (recommended)
htpasswd -B passwords operator
htpasswd -B passwords emergency
htpasswd -B passwords monitoring

# Add user with MD5 (more compatible)
htpasswd -m passwords legacy_system

# For system service installation
sudo -u hackrf htpasswd -c /var/lib/hackrf-server/passwords admin
sudo -u hackrf htpasswd -B /var/lib/hackrf-server/passwords operator

# Verify passwords
htpasswd -v passwords admin
htpasswd -v passwords operator
```

### Password File Management
```bash
# View all users
cut -d: -f1 passwords

# Delete a user
htpasswd -D passwords old_user

# Update password
htpasswd -B passwords existing_user

# Secure the file
chmod 600 passwords
chown hackrf:hackrf passwords  # If running as hackrf user

# For system service
sudo chmod 600 /var/lib/hackrf-server/passwords
sudo chown hackrf:hackrf /var/lib/hackrf-server/passwords
```

## Advanced Configuration

### Custom Configuration Files
```bash
# Create production config
cat > production.config << EOF
BIND_ADDRESS=0.0.0.0
SERIAL_LISTEN_PORT=16175
HTTP_LISTEN_PORT=16180
SAMPLE_RATE=8000000
BITRATE=3200
AMPLITUDE=100
TX_GAIN=30
DEFAULT_FREQUENCY=931937500
EOF

# Use custom config (rename to config.ini)
mv production.config config.ini
./hackrf_http_server --verbose
```

### Environment Variable Override
```bash
# Override specific settings
export TX_GAIN=25
export AMPLITUDE=90
export DEFAULT_FREQUENCY=925516000
./hackrf_http_server --verbose

# Disable serial protocol
export SERIAL_LISTEN_PORT=0
./hackrf_http_server --verbose

# HTTP only configuration
export SERIAL_LISTEN_PORT=0
export HTTP_LISTEN_PORT=16180
export BIND_ADDRESS=127.0.0.1
./hackrf_http_server --verbose
```

## System Service Examples

### Service Management
```bash
# Install and setup service
sudo make install
sudo -u hackrf htpasswd -c /var/lib/hackrf-server/passwords admin

# Service control
sudo make start-service
sudo make status-service
sudo make logs

# Configuration changes
sudo nano /etc/default/hackrf_http_server
sudo make restart-service
```

### Service Testing
```bash
# Test service with curl
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Service test message"}'

# Monitor service logs
sudo journalctl -u hackrf-http-server -f

# Check service status
sudo systemctl status hackrf-http-server
```

## Monitoring and Logging

### Comprehensive Logging
```bash
# Full verbose logging to file
./hackrf_http_server --verbose 2>&1 | tee hackrf_server.log

# Monitor in real-time
tail -f hackrf_server.log

# Filter for specific events
grep "Message Processing" hackrf_server.log
grep "HTTP Response" hackrf_server.log
grep "ERROR\|Missing required field" hackrf_server.log
grep "Authentication" hackrf_server.log
```

### Statistics and Analysis
```bash
# Count successful transmissions
grep -c "Message Processing Completed" hackrf_server.log

# Show HTTP response codes
grep "HTTP Response sent" hackrf_server.log | grep -o "Status: [0-9]*" | sort | uniq -c

# Show validation failures
grep "Missing required field" hackrf_server.log | wc -l

# Show client connections
grep "Client connected" hackrf_server.log

# Extract transmission times
grep "transmission time" hackrf_server.log | grep -o "[0-9]*\.[0-9]* ms"

# Count different types of errors
echo "Missing capcode errors:"
grep -c "Missing required field: capcode" hackrf_server.log

echo "Missing message errors:"
grep -c "Missing required field: message" hackrf_server.log

echo "Authentication failures:"
grep -c "unauthorized\|Authentication failed" hackrf_server.log
```

## Debugging and Development

### Debug Mode Usage
```bash
# Generate IQ files without transmission
./hackrf_http_server --debug --verbose

# Send test message to generate IQ file
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Debug test message"}'

# Analyze generated IQ file with GNU Radio or other tools
ls -la flexserver_output.iq
file flexserver_output.iq
```

### Signal Analysis
```bash
# View IQ file properties
file flexserver_output.iq
ls -lh flexserver_output.iq

# Convert to different formats (if needed)
# Note: This requires additional tools like GNU Radio

# Multiple debug runs
for i in {1..5}; do
    curl -X POST http://localhost:16180/ \
      -u admin:passw0rd \
      -H "Content-Type: application/json" \
      -d "{\"capcode\": $((1000 + i)), \"message\": \"Debug message $i\"}"

    mv flexserver_output.iq debug_message_$i.iq
done
```

### Testing Without HackRF
```bash
# Debug mode doesn't require HackRF device
./hackrf_http_server --debug --verbose
# Perfect for development and testing

# Test all validation scenarios
echo "Testing required field validation..."

# Test valid message
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Valid message"}' | jq .

# Test missing capcode
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"message": "Missing capcode"}' | jq .

# Test missing message
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334}' | jq .
```

## Integration Examples

### Shell Script Integration
```bash
#!/bin/bash
# monitoring_alert.sh - System monitoring alert script

ALERT_URL="http://localhost:16180/"
AUTH="admin:passw0rd"

send_alert() {
    local capcode="$1"
    local message="$2"
    local frequency="$3"

    # Validate required parameters
    if [ -z "$capcode" ] || [ -z "$message" ]; then
        echo "Error: Both capcode and message are required"
        echo "Usage: send_alert CAPCODE MESSAGE [FREQUENCY]"
        return 1
    fi

    # Build JSON payload
    local payload="{\"capcode\": $capcode, \"message\": \"$message\""
    if [ -n "$frequency" ]; then
        payload="$payload, \"frequency\": $frequency"
    fi
    payload="$payload}"

    response=$(curl -s -w "%{http_code}" -X POST "$ALERT_URL" \
        -u "$AUTH" \
        -H "Content-Type: application/json" \
        -d "$payload")

    http_code="${response: -3}"
    response_body="${response%???}"

    if [ "$http_code" = "200" ]; then
        echo "Alert sent successfully"
        return 0
    else
        echo "Alert failed with HTTP code: $http_code"
        echo "Response: $response_body"
        return 1
    fi
}

# Usage examples with validation
send_alert 11111 "System startup completed"
send_alert 22222 "High CPU usage detected" 925516000
send_alert 33333 "Backup completed successfully"

# These will fail with usage message
# send_alert 11111  # Missing message
# send_alert "" "Missing capcode"  # Empty capcode
```

### Python Integration
```python
#!/usr/bin/env python3
# hackrf_client.py - Python client for HackRF server

import requests
import json
import sys
from requests.auth import HTTPBasicAuth

class HackRFClient:
    def __init__(self, base_url="http://localhost:16180", username="admin", password="passw0rd"):
        self.base_url = base_url
        self.auth = HTTPBasicAuth(username, password)
        self.headers = {"Content-Type": "application/json"}

    def send_message(self, capcode, message, frequency=None):
        """
        Send a message via HTTP API

        Args:
            capcode (int): REQUIRED - Target pager capcode
            message (str): REQUIRED - Message text to send
            frequency (int, optional): Transmission frequency, uses server default if None

        Returns:
            dict: Response with success status and details
        """
        # Validate required parameters
        if not capcode or capcode == 0:
            return {
                "success": False,
                "error": "capcode is required and must be non-zero"
            }

        if not message or message.strip() == "":
            return {
                "success": False,
                "error": "message is required and cannot be empty"
            }

        # Build payload with required fields
        payload = {
            "capcode": int(capcode),
            "message": str(message)
        }

        # Add optional frequency
        if frequency and frequency > 0:
            payload["frequency"] = int(frequency)

        try:
            response = requests.post(
                self.base_url,
                auth=self.auth,
                headers=self.headers,
                json=payload,
                timeout=30
            )

            return {
                "success": response.status_code == 200,
                "status_code": response.status_code,
                "response": response.json() if response.headers.get("content-type", "").startswith("application/json") else response.text
            }

        except requests.exceptions.RequestException as e:
            return {
                "success": False,
                "error": str(e)
            }

    def test_connection(self):
        """Test server connectivity"""
        try:
            response = requests.get(self.base_url, timeout=5)
            return response.status_code == 405  # Should return Method Not Allowed for GET
        except:
            return False

    def validate_message(self, capcode, message, frequency=None):
        """
        Validate message parameters without sending

        Returns:
            tuple: (is_valid, error_message)
        """
        if not capcode or capcode == 0:
            return False, "capcode is required and must be non-zero"

        if not message or message.strip() == "":
            return False, "message is required and cannot be empty"

        if frequency is not None and frequency <= 0:
            return False, "frequency must be positive if specified"

        return True, None

# Usage examples and testing
if __name__ == "__main__":
    client = HackRFClient()

    # Test connection
    if not client.test_connection():
        print("ERROR: Cannot connect to HackRF server")
        sys.exit(1)

    print("Testing HackRF Client with required field validation...")

    # Test valid messages
    test_cases = [
        # (capcode, message, frequency, expected_success)
        (1122334, "Python test message", None, True),
        (911911, "Emergency alert", 931937500, True),
        (99999, "Status check", 925516000, True),

        # Test invalid cases
        (0, "Zero capcode", None, False),
        (1122334, "", None, False),
        (1122334, "   ", None, False),  # Whitespace only
        (None, "No capcode", None, False),
    ]

    for capcode, message, frequency, should_succeed in test_cases:
        print(f"\nTesting: capcode={capcode}, message='{message}', frequency={frequency}")

        # Validate first
        valid, error = client.validate_message(capcode, message, frequency)
        print(f"Validation: {'PASS' if valid else 'FAIL'}")
        if not valid:
            print(f"Validation error: {error}")
            continue

        # Send message
        result = client.send_message(capcode, message, frequency)
        success = result["success"]

        print(f"Send result: {'SUCCESS' if success else 'FAILED'}")
        if not success:
            print(f"Error: {result.get('error', result.get('response'))}")

        # Check if result matches expectation
        if success == should_succeed:
            print("✓ Test passed")
        else:
            print("✗ Test failed - unexpected result")

    print("\n" + "="*50)
    print("Testing edge cases...")

    # Test very long message
    long_message = "A" * 1000
    result = client.send_message(1122334, long_message)
    print(f"Long message result: {'SUCCESS' if result['success'] else 'FAILED'}")

    # Test special characters
    special_message = "Test with émojis 🚨 and spëcial chars: äöü"
    result = client.send_message(1122334, special_message)
    print(f"Special chars result: {'SUCCESS' if result['success'] else 'FAILED'}")
```

### Batch Processing Example
```python
#!/usr/bin/env python3
# batch_pager.py - Send multiple messages efficiently

import requests
import json
import time
from concurrent.futures import ThreadPoolExecutor
from requests.auth import HTTPBasicAuth

class BatchPager:
    def __init__(self, server_url, username, password, max_workers=5):
        self.server_url = server_url
        self.auth = HTTPBasicAuth(username, password)
        self.headers = {"Content-Type": "application/json"}
        self.max_workers = max_workers

    def send_single_message(self, message_data):
        """Send a single message with required field validation"""
        # Validate required fields
        if 'capcode' not in message_data or message_data['capcode'] == 0:
            return {
                'message_data': message_data,
                'success': False,
                'error': 'capcode is required and must be non-zero'
            }

        if 'message' not in message_data or not message_data['message'].strip():
            return {
                'message_data': message_data,
                'success': False,
                'error': 'message is required and cannot be empty'
            }

        try:
            response = requests.post(
                f'{self.server_url}/',
                auth=self.auth,
                headers=self.headers,
                json=message_data,
                timeout=30
            )
            return {
                'message_data': message_data,
                'success': response.status_code == 200,
                'response': response.text,
                'status_code': response.status_code
            }
        except Exception as e:
            return {
                'message_data': message_data,
                'success': False,
                'error': str(e)
            }

    def send_batch(self, messages, delay_between_messages=0.1):
        """Send multiple messages with validation"""
        results = []

        with ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            futures = []

            for message in messages:
                future = executor.submit(self.send_single_message, message)
                futures.append(future)

                # Add delay to prevent overwhelming the server
                if delay_between_messages > 0:
                    time.sleep(delay_between_messages)

            # Collect results
            for future in futures:
                results.append(future.result())

        return results

# Usage example
if __name__ == "__main__":
    pager = BatchPager('http://localhost:16180', 'admin', 'passw0rd')

    # Test messages with required fields
    messages = [
        # Valid messages
        {'capcode': 1000001, 'message': 'System maintenance starting'},
        {'capcode': 1000002, 'message': 'Database backup in progress', 'frequency': 925516000},
        {'capcode': 1000003, 'message': 'Maintenance completed successfully'},

        # Invalid messages (will fail validation)
        {'message': 'Missing capcode'},  # No capcode
        {'capcode': 1000004},  # No message
        {'capcode': 0, 'message': 'Zero capcode'},  # Zero capcode
        {'capcode': 1000005, 'message': ''},  # Empty message
    ]

    print("Sending batch of messages...")
    results = pager.send_batch(messages)

    # Print results
    success_count = 0
    failure_count = 0

    for result in results:
        if result['success']:
            success_count += 1
            capcode = result['message_data'].get('capcode', 'N/A')
            message = result['message_data'].get('message', 'N/A')
            print(f"✓ SUCCESS: Capcode {capcode} - '{message}'")
        else:
            failure_count += 1
            capcode = result['message_data'].get('capcode', 'N/A')
            message = result['message_data'].get('message', 'N/A')
            error = result.get('error', result.get('response', 'Unknown error'))
            print(f"✗ FAILED: Capcode {capcode} - '{message}' - Error: {error}")

    print(f"\nBatch Summary: {success_count} successful, {failure_count} failed")
```

### Cron Integration
```bash
# Add to crontab for scheduled messages
# crontab -e

# Send hourly status message (TCP protocol)
0 * * * * echo '99999|Hourly status check - $(date)|931937500' | nc localhost 16175

# Send daily backup notification (HTTP protocol with required fields)
0 2 * * * curl -s -X POST http://localhost:16180/ -u admin:passw0rd -H "Content-Type: application/json" -d '{"capcode": 88888, "message": "Daily backup starting"}'

# Weekly system health check
0 9 * * 1 /usr/local/bin/system_health_check.sh

# Example health check script
cat > /usr/local/bin/system_health_check.sh << 'EOF'
#!/bin/bash
# system_health_check.sh

CAPCODE=77777
SERVER_URL="http://localhost:16180/"
AUTH="admin:passw0rd"

# Check system metrics
CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1)
MEMORY_USAGE=$(free | grep Mem | awk '{printf("%.1f"), $3/$2 * 100.0}')
DISK_USAGE=$(df / | tail -1 | awk '{print $5}' | cut -d'%' -f1)

# Build message
MESSAGE="Weekly Health: CPU:${CPU_USAGE}% MEM:${MEMORY_USAGE}% DISK:${DISK_USAGE}%"

# Send alert (with required capcode and message fields)
curl -s -X POST "$SERVER_URL" \
  -u "$AUTH" \
  -H "Content-Type: application/json" \
  -d "{\"capcode\": $CAPCODE, \"message\": \"$MESSAGE\"}"
EOF

chmod +x /usr/local/bin/system_health_check.sh
```

## Performance and Production

### High-Volume Usage
```bash
# For high message volume, consider:
# 1. Increase sample rate for better quality
export SAMPLE_RATE=8000000

# 2. Use higher bitrate
export BITRATE=3200

# 3. Monitor system resources
./hackrf_http_server --verbose &
SERVER_PID=$!

# Monitor CPU and memory usage
while kill -0 $SERVER_PID 2>/dev/null; do
    ps -p $SERVER_PID -o %cpu,%mem,cmd
    sleep 60
done
```

### Load Testing with Required Fields
```bash
#!/bin/bash
# load_test.sh - Load test with proper validation

TOTAL_MESSAGES=100
CONCURRENT_JOBS=10
SUCCESS_COUNT=0
FAILURE_COUNT=0

echo "Starting load test with $TOTAL_MESSAGES messages..."

for i in $(seq 1 $TOTAL_MESSAGES); do
    # Send message with required fields
    response=$(curl -s -w "%{http_code}" -X POST http://localhost:16180/ \
        -u admin:passw0rd \
        -H "Content-Type: application/json" \
        -d "{\"capcode\": $((1000 + i)), \"message\": \"Load test message $i\"}")

    http_code="${response: -3}"

    if [ "$http_code" = "200" ]; then
        ((SUCCESS_COUNT++))
        echo -n "."
    else
        ((FAILURE_COUNT++))
        echo -n "x"
    fi

    # Add small delay to prevent overwhelming
    sleep 0.1

    if [ $((i % 50)) -eq 0 ]; then
        echo " [$i/$TOTAL_MESSAGES]"
    fi
done

echo ""
echo "Load test completed:"
echo "  Successful: $SUCCESS_COUNT"
echo "  Failed: $FAILURE_COUNT"
echo "  Success rate: $(( SUCCESS_COUNT * 100 / TOTAL_MESSAGES ))%"
```

## Troubleshooting Examples

### Validation Testing
```bash
#!/bin/bash
# validation_test.sh - Test all validation scenarios

echo "Testing field validation..."

# Test 1: Valid message
echo "Test 1: Valid message"
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Valid message"}' | jq .

# Test 2: Missing capcode
echo -e "\nTest 2: Missing capcode (should fail)"
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"message": "Missing capcode"}' | jq .

# Test 3: Missing message
echo -e "\nTest 3: Missing message (should fail)"
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334}' | jq .

# Test 4: Zero capcode
echo -e "\nTest 4: Zero capcode (should fail)"
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 0, "message": "Zero capcode"}' | jq .

# Test 5: Empty message
echo -e "\nTest 5: Empty message (should fail)"
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": ""}' | jq .

# Test 6: Optional frequency (should succeed)
echo -e "\nTest 6: With optional frequency"
curl -s -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "With frequency", "frequency": 925516000}' | jq .

echo -e "\nValidation testing completed!"
```

### Network Issues
```bash
# Check if ports are in use
netstat -tlnp | grep :16180
netstat -tlnp | grep :16175

# Test port connectivity
nc -zv localhost 16180
nc -zv localhost 16175

# Check firewall (if applicable)
sudo ufw status
sudo iptables -L -n

# Test with minimal valid request
echo '{"capcode": 1, "message": "test"}' | \
  curl -X POST http://localhost:16180/ \
    -u admin:passw0rd \
    -H "Content-Type: application/json" \
    -d @-
```

### HackRF Issues
```bash
# Check HackRF device
hackrf_info

# Check USB connection
lsusb | grep -i hackrf

# Test HackRF transmission
hackrf_transfer -t /dev/zero -f 915000000 -s 2000000 -x 0

# Check permissions
ls -la /dev/bus/usb/*/*
groups $USER  # Should include 'plugdev'

# Test with debug mode (no HackRF needed)
./hackrf_http_server --debug --verbose
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Debug test"}'
```

### Authentication Issues
```bash
# Verify password file
ls -la passwords
file passwords

# Test password verification
htpasswd -v passwords admin

# Check file permissions
chmod 600 passwords

# Test authentication with verbose output
curl -v -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Auth test"}'

# Test wrong credentials
curl -v -X POST http://localhost:16180/ \
  -u admin:wrongpass \
  -H "Content-Type: application/json" \
  -d '{"capcode": 1122334, "message": "Should fail"}'
```

This comprehensive examples guide demonstrates proper usage of the enhanced HackRF HTTP/TCP Server with mandatory capcode and message field validation, helping users understand all features and avoid common validation errors.
