# HackRF Grafana Webhook Service

This service bridges Grafana Alertmanager webhooks to the HackRF FLEX paging system.

## Features
- Receives Grafana webhook alerts
- Forwards alerts to HackRF server
- Configurable via CLI, environment, or config file
- HTTPS support with automatic port switching
- Comprehensive logging

## Installation

### 1. Prerequisites
- Python 3.8+
- HackRF server running
- Grafana Alertmanager configured

### 2. Install Service
```bash
sudo mkdir -p /opt/hackrf-grafana-webhook
sudo cp hackrf-grafana-webhook.py /opt/hackrf-grafana-webhook/
sudo chmod +x /opt/hackrf-grafana-webhook/hackrf-grafana-webhook.py
```

### 3. Create service user
```bash
sudo useradd -r -s /usr/sbin/nologin hackrf
sudo chown -R hackrf:hackrf /opt/hackrf-grafana-webhook
```

### 4. Configuration
Create configuration directory and generate default config:
```bash
sudo mkdir -p /etc/hackrf-grafana-webhook
sudo /opt/hackrf-grafana-webhook/hackrf-grafana-webhook.py \
  --generate-config /etc/hackrf-grafana-webhook/hackrf-grafana-webhook.cfg
```

### 5. Set ownership
```bash
sudo chown -R hackrf:hackrf /etc/hackrf-grafana-webhook
```

### 6. Edit configuration (if needed)
```bash
sudo nano /etc/hackrf-grafana-webhook/hackrf-grafana-webhook.cfg
```

### 7. Systemd Service
Install service file:
```bash
sudo cp hackrf-grafana-webhook.service /etc/systemd/system/
sudo systemctl daemon-reload
```

Environment variables (optional):

Create environment file
```bash
sudo nano /etc/default/hackrf-grafana-webhook
```

Add any overrides (these take priority over config file)
```
HACKRF_SERVER_URL=http://localhost:16180
HACKRF_USERNAME=admin
HACKRF_PASSWORD=your_secure_password
```

### 8. Start Service
```bash
sudo systemctl enable hackrf-grafana-webhook
sudo systemctl start hackrf-grafana-webhook
sudo systemctl status hackrf-grafana-webhook
```

# Usage

## Command Line Options
```bash
$ ./hackrf-grafana-webhook.py --help

Usage: hackrf-grafana-webhook.py [-h] [-c FILE] [-g FILE] [-v]

Options:
  -h, --help            show this help message and exit
  -c FILE, --config FILE
                        Path to configuration file (default: /etc/hackrf-grafana-webhook/hackrf-grafana-webhook.cfg)
  -g FILE, --generate-config FILE
                        Generate default configuration file
  -v, --verbose         Enable verbose/debug mode
```

## Configuration Priority:
  1. Command-line arguments
  2. Environment variables
  3. Configuration file
  4. Internal defaults

## Environment Variables:
```
  HACKRF_SERVER_URL    HackRF server URL
  HACKRF_USERNAME      HackRF server username
  HACKRF_PASSWORD      HackRF server password
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

## Webhook Endpoints

  POST `/api/v1/alerts` - Main webhook endpoint

  POST `/` - Alternative endpoint

  GET `/health` - Health check endpoint

## Grafana Alert Configuration
Configure Grafana Alertmanager with:
```
receivers:
  - name: 'hackrf-pager'
    webhook_configs:
      - url: 'http://your-server:8080/api/v1/alerts'  # Use 8443 for HTTPS
        send_resolved: true
```
## Security Recommendations
  - Use HTTPS in production
  - Set strong passwords
  - Restrict firewall access to webhook port
  - Regularly rotate SSL certificates

## Troubleshooting
View logs:
```bash
journalctl -u hackrf-grafana-webhook -f
```

Test with curl:
```bash
curl -X POST -H "Content-Type: application/json" \
  -d '[{"labels":{"alertname":"TestAlert","capcode":"12345"},"annotations":{"summary":"Test message"}}]' \
  http://localhost:8080/api/v1/alerts
```

## Generating SSL Certificates

```bash
sudo openssl req -x509 -newkey rsa:4096 \
  -keyout /etc/ssl/private/hackrf-grafana-webhook.key \
  -out /etc/ssl/certs/hackrf-grafana-webhook.crt \
  -days 365 -nodes \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=grafana-flex-webhook"

sudo chmod 600 /etc/ssl/private/hackrf-grafana-webhook.key
sudo chmod 644 /etc/ssl/certs/hackrf-grafana-webhook.crt
sudo chown hackrf:hackrf /etc/ssl/private/hackrf-grafana-webhook.key
sudo chown hackrf:hackrf /etc/ssl/certs/hackrf-grafana-webhook.crt
```
