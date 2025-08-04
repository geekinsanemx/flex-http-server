# Grafana Webhook Service for FLEX Paging

Universal webhook service that bridges Grafana Alertmanager to any FLEX paging HTTP server (HackRF or TTGO implementations).

## Features
- Receives Grafana webhook alerts
- Forwards alerts to any FLEX HTTP server (HackRF/TTGO)
- Universal compatibility with both backend implementations
- Configurable via CLI, environment, or config file
- HTTPS support with automatic port switching
- Comprehensive logging and debugging

## Compatibility

This webhook service works with both FLEX HTTP server implementations:

- **HackRF HTTP Server** - High-performance SDR-based implementation
- **TTGO HTTP Server** - Cost-effective ESP32-based implementation

Both servers use the same HTTP API format, making this webhook universally compatible.

## Installation

### 1. Prerequisites
- Python 3.8+
- Either HackRF or TTGO HTTP server running
- Grafana Alertmanager configured

### 2. Install Service
```bash
sudo mkdir -p /opt/grafana-webhook
sudo cp grafana-webhook.py /opt/grafana-webhook/
sudo chmod +x /opt/grafana-webhook/grafana-webhook.py
```

### 3. Create service user
```bash
sudo useradd -r -s /usr/sbin/nologin grafana
sudo chown -R grafana:grafana /opt/grafana-webhook
```

### 4. Configuration
Create configuration directory and generate default config:
```bash
sudo mkdir -p /etc/grafana-webhook
sudo /opt/grafana-webhook/grafana-webhook.py \
  --generate-config /etc/grafana-webhook/grafana-webhook.cfg
```

### 5. Set ownership
```bash
sudo chown -R grafana:grafana /etc/grafana-webhook
```

### 6. Edit configuration
```bash
sudo nano /etc/grafana-webhook/grafana-webhook.cfg
```

Configure the FLEX server URL to point to your backend:
```ini
[FLEX]
# For HackRF server
FLEX_SERVER_URL = http://localhost:16180

# For TTGO server (same URL, different backend)
FLEX_SERVER_URL = http://localhost:16180

# Or remote server
FLEX_SERVER_URL = http://192.168.1.100:16180
```

### 7. Systemd Service
Install service file:
```bash
sudo cp grafana-webhook.service /etc/systemd/system/
sudo systemctl daemon-reload
```

Environment variables (optional):

Create environment file:
```bash
sudo nano /etc/default/grafana-webhook
```

Add any overrides (these take priority over config file):
```bash
FLEX_SERVER_URL=http://localhost:16180
FLEX_USERNAME=admin
FLEX_PASSWORD=your_secure_password
```

### 8. Start Service
```bash
sudo systemctl enable grafana-webhook
sudo systemctl start grafana-webhook
sudo systemctl status grafana-webhook
```

## Usage

### Command Line Options
```bash
$ ./grafana-webhook.py --help

Usage: grafana-webhook.py [-h] [-c FILE] [-g FILE] [-v]

Options:
  -h, --help            show this help message and exit
  -c FILE, --config FILE
                        Path to configuration file (default: /etc/grafana-webhook/grafana-webhook.cfg)
  -g FILE, --generate-config FILE
                        Generate default configuration file
  -v, --verbose         Enable verbose/debug mode
```

### Configuration Priority:
1. Command-line arguments
2. Environment variables
3. Configuration file
4. Internal defaults

### Environment Variables:
```
FLEX_SERVER_URL      FLEX server URL (HackRF or TTGO)
FLEX_USERNAME        FLEX server username
FLEX_PASSWORD        FLEX server password
DEFAULT_CAPCODE      Default capcode for alerts
DEFAULT_FREQUENCY    Default frequency for alerts (Hz)
REQUEST_TIMEOUT      Network timeout (seconds)
BIND_HOST            Flask bind host
BIND_PORT            Flask bind port
SSL_CERT_PATH        SSL certificate path
SSL_KEY_PATH         SSL private key path
LOG_LEVEL            Logging level (DEBUG, INFO, WARNING, ERROR)
DEBUG_MODE           Enable debug mode (True/False)
```

## API Endpoints

### Webhook Endpoints
- `POST /api/v1/alerts` - Main webhook endpoint
- `POST /` - Alternative endpoint
- `GET /health` - Health check endpoint

### Health Check Response
```json
{
  "status": "healthy",
  "timestamp": "2024-01-01T12:00:00.000000",
  "service": "grafana-webhook",
  "port": 8080,
  "ssl": false,
  "flex_server": "http://localhost:16180"
}
```

## Grafana Configuration

### Alertmanager Configuration
Configure Grafana Alertmanager with:
```yaml
receivers:
  - name: 'flex-pager'
    webhook_configs:
      - url: 'http://your-server:8080/api/v1/alerts'  # Use 8443 for HTTPS
        send_resolved: true
```

### Alert Labels for Custom Routing
You can customize paging behavior using alert labels:

```yaml
# Alert rule with custom capcode and frequency
- alert: ServerDown
  expr: up == 0
  labels:
    severity: critical
    capcode: "911911"           # Custom capcode for this alert
    frequency: "931937500"      # Custom frequency
  annotations:
    summary: "Server {{ $labels.instance }} is down"
```

### Supported Label Keys
The webhook looks for these labels in priority order:

**Capcode**: `capcode`, `pager_capcode`, `flex_capcode`  
**Frequency**: `frequency`, `pager_frequency`, `flex_frequency`

If not found, uses default values from configuration.

## Backend Server Compatibility

### HackRF Server Integration
```ini
[FLEX]
FLEX_SERVER_URL = http://localhost:16180
FLEX_USERNAME = admin
FLEX_PASSWORD = passw0rd
DEFAULT_FREQUENCY = 931937500  # VHF/UHF paging frequency
```

### TTGO Server Integration
```ini
[FLEX]
FLEX_SERVER_URL = http://localhost:16180
FLEX_USERNAME = admin  
FLEX_PASSWORD = passw0rd
DEFAULT_FREQUENCY = 915000000  # ISM band frequency
```

Both configurations use identical API calls - the webhook automatically adapts to whichever server is running.

## Testing

### Manual Testing
Test the webhook with curl:
```bash
# Test basic alert
curl -X POST -H "Content-Type: application/json" \
  -d '[{
    "labels": {
      "alertname": "TestAlert",
      "capcode": "12345"
    },
    "annotations": {
      "summary": "Test message from webhook"
    }
  }]' \
  http://localhost:8080/api/v1/alerts

# Test with custom frequency
curl -X POST -H "Content-Type: application/json" \
  -d '[{
    "labels": {
      "alertname": "CustomFreqTest",
      "capcode": "911911",
      "frequency": "915000000"
    },
    "annotations": {
      "summary": "Test with custom frequency"
    }
  }]' \
  http://localhost:8080/api/v1/alerts
```

### Health Check
```bash
curl http://localhost:8080/health
```

Expected response:
```json
{
  "status": "healthy",
  "timestamp": "2024-01-01T12:00:00.000000",
  "service": "grafana-webhook",
  "port": 8080,
  "ssl": false,
  "flex_server": "http://localhost:16180"
}
```

## Security

### HTTPS Configuration
For production deployments, enable HTTPS:

```ini
[SSL]
SSL_CERT_PATH = /etc/ssl/certs/grafana-webhook.crt
SSL_KEY_PATH = /etc/ssl/private/grafana-webhook.key
```

When SSL is enabled, the service automatically switches to port 8443.

### Generate SSL Certificates
```bash
sudo openssl req -x509 -newkey rsa:4096 \
  -keyout /etc/ssl/private/grafana-webhook.key \
  -out /etc/ssl/certs/grafana-webhook.crt \
  -days 365 -nodes \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=grafana-webhook"

sudo chmod 600 /etc/ssl/private/grafana-webhook.key
sudo chmod 644 /etc/ssl/certs/grafana-webhook.crt
sudo chown grafana:grafana /etc/ssl/private/grafana-webhook.key
sudo chown grafana:grafana /etc/ssl/certs/grafana-webhook.crt
```

### Security Recommendations
- Use HTTPS in production
- Set strong passwords for FLEX server authentication
- Restrict firewall access to webhook port
- Regularly rotate SSL certificates
- Use strong authentication between Grafana and webhook service

## Troubleshooting

### View Service Logs
```bash
# Real-time logs
sudo journalctl -u grafana-webhook -f

# Recent logs
sudo journalctl -u grafana-webhook --since "1 hour ago"

# Debug mode logs
sudo systemctl stop grafana-webhook
sudo -u grafana /usr/local/bin/grafana-webhook --verbose
```

### Common Issues

#### Connection Refused to FLEX Server
```bash
# Check if FLEX server is running
curl -u admin:passw0rd http://localhost:16180/

# For HackRF server
sudo systemctl status hackrf-http-server

# For TTGO server  
sudo systemctl status ttgo-http-server
```

#### Authentication Failures
```bash
# Test FLEX server credentials
curl -u admin:passw0rd -X POST \
  -H "Content-Type: application/json" \
  -d '{"capcode": 12345, "message": "test"}' \
  http://localhost:16180/
```

#### Port Binding Issues
```bash
# Check port usage
netstat -tlnp | grep :8080

# Check service configuration
sudo systemctl cat grafana-webhook
```

#### SSL Certificate Issues
```bash
# Verify certificate files
sudo ls -la /etc/ssl/certs/grafana-webhook.crt
sudo ls -la /etc/ssl/private/grafana-webhook.key

# Test SSL configuration
openssl x509 -in /etc/ssl/certs/grafana-webhook.crt -text -noout
```

### Debug Mode Testing
Run in debug mode to see detailed processing:
```bash
sudo -u grafana /usr/local/bin/grafana-webhook --verbose --config /etc/grafana-webhook/grafana-webhook.cfg
```

This shows:
- Incoming webhook payloads
- Alert processing steps
- FLEX server communication
- Response handling

## Migration from hackrf-grafana-webhook

If upgrading from the old `hackrf-grafana-webhook` service:

### 1. Stop old service
```bash
sudo systemctl stop hackrf-grafana-webhook
sudo systemctl disable hackrf-grafana-webhook
```

### 2. Migrate configuration
```bash
# Copy and rename config
sudo cp /etc/hackrf-grafana-webhook/hackrf-grafana-webhook.cfg \
        /etc/grafana-webhook/grafana-webhook.cfg

# Update config file (change HACKRF_* to FLEX_*)
sudo sed -i 's/HACKRF_/FLEX_/g' /etc/grafana-webhook/grafana-webhook.cfg
sudo sed -i 's/\[HACKRF\]/[FLEX]/g' /etc/grafana-webhook/grafana-webhook.cfg
```

### 3. Update Grafana configuration
Update your Grafana webhook URLs from:
```
http://your-server:8080/api/v1/alerts
```
(No URL change needed - same endpoints)

### 4. Start new service
```bash
sudo systemctl enable grafana-webhook
sudo systemctl start grafana-webhook
```

## Integration Examples

### Python Client
```python
import requests

def test_webhook():
    webhook_url = "http://localhost:8080/api/v1/alerts"

    alerts = [{
        "labels": {
            "alertname": "TestAlert",
            "capcode": "12345",
            "frequency": "915000000"
        },
        "annotations": {
            "summary": "Test message from Python"
        }
    }]

    response = requests.post(webhook_url, json=alerts)
    return response.status_code == 200

# Test the webhook
if test_webhook():
    print("Webhook test successful")
else:
    print("Webhook test failed")
```

### Shell Script Integration
```bash
#!/bin/bash
# send_test_alert.sh

WEBHOOK_URL="http://localhost:8080/api/v1/alerts"

send_test_alert() {
    local capcode="$1"
    local message="$2"
    local frequency="${3:-915000000}"

    payload='[{
        "labels": {
            "alertname": "ScriptAlert",
            "capcode": "'$capcode'",
            "frequency": "'$frequency'"
        },
        "annotations": {
            "summary": "'$message'"
        }
    }]'

    curl -X POST -H "Content-Type: application/json" \
         -d "$payload" "$WEBHOOK_URL"
}

# Usage examples
send_test_alert "12345" "System maintenance starting"
send_test_alert "911911" "Emergency: Server failure" "931937500"
```

## Advanced Configuration

### Multiple FLEX Servers
For load balancing or redundancy, you can run multiple webhook instances pointing to different FLEX servers:

```bash
# Primary webhook (HackRF)
sudo systemctl start grafana-webhook@primary

# Secondary webhook (TTGO)
sudo systemctl start grafana-webhook@secondary
```

Create service templates with different config files for each backend.

### Custom Message Formatting
The webhook uses this priority for message content:
1. `annotations.summary`
2. `annotations.description`
3. `annotations.message`
4. Default: "Alert triggered"

Final message format: `[STATUS] AlertName: Content`

Where STATUS is either "FIRING" or "RESOLVED" based on alert state.

## Related Documentation

- [HackRF HTTP Server](../hackrf_http_server/README.md) - High-performance SDR backend
- [TTGO HTTP Server](../ttgo_http_server/README.md) - ESP32-based backend
- [Main Repository](../../README.md) - Overview of all implementations

## Support

For webhook-specific issues:
1. Check the service logs: `sudo journalctl -u grafana-webhook -f`
2. Test FLEX server connectivity manually
3. Verify Grafana webhook configuration
4. Run in debug mode for detailed troubleshooting

For FLEX server issues, see the respective backend documentation.
