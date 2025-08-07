# flex-http-server

A comprehensive collection of dual-protocol HTTP/TCP servers for transmitting FLEX paging messages via different hardware platforms and integration methods.

## Overview

This repository provides multiple implementations of FLEX paging servers, each optimized for specific hardware platforms and use cases. All implementations support both HTTP REST API and legacy TCP protocols for maximum compatibility with existing systems and modern cloud integrations.

FLEX (FLexible EXchange) is a digital paging protocol developed by Motorola that operates in the VHF and UHF bands, commonly used for numeric and alphanumeric paging services.

## Repository Structure

### 🎯 [HackRF HTTP Server](./hackrf_http_server/)
**Advanced software-defined radio implementation**

A full-featured FLEX paging server for HackRF devices with enterprise-grade features:
- **Hardware**: HackRF One and compatible SDR devices
- **Protocols**: HTTP JSON API + legacy TCP serial protocol
- **Security**: HTTP Basic Authentication with htpasswd compatibility
- **Features**: Emergency Message Resynchronization (EMR), comprehensive verbose logging, debug mode
- **Integration**: AWS Lambda compatible response codes, systemd service support
- **Use Case**: High-performance, production-ready paging infrastructure

[📖 See detailed documentation](./hackrf_http_server/README.md)

### 📡 [TTGO HTTP Server](./ttgo_http_server/)
**Cost-effective ESP32-based implementation**

A lightweight FLEX paging server for TTGO ESP32 + SX127x modules:
- **Hardware**: TTGO LoRa32 V1/V2, TTGO T-Beam, TTGO LoRa32-OLED with ttgo-fsk-tx firmware
- **Protocols**: HTTP JSON API + legacy TCP protocol
- **Features**: Direct ESP32 communication, multiple ISM bands support (433/868/915 MHz)
- **Dependencies**: Requires [ttgo-fsk-tx firmware](https://github.com/rlaneth/ttgo-fsk-tx/)
- **Use Case**: Portable, low-cost paging solutions, IoT integration

[📖 See detailed documentation](./ttgo_http_server/README.md)

### 🔧 [FLEX HTTP Server](./flex_http_server/)
**Modern AT command-based implementation**

An AT command-based FLEX paging server for broader hardware compatibility:
- **Hardware**: TTGO LoRa32-OLED, Heltec WiFi LoRa 32 V3, Heltec LoRa32 V3 with flex-fsk-tx firmware
- **Protocols**: HTTP JSON API + legacy TCP protocol
- **Features**: Standard AT command interface, universal device compatibility
- **Dependencies**: Requires [flex-fsk-tx firmware](https://github.com/geekinsanemx/flex-fsk-tx/)
- **Use Case**: Modern implementation with broader hardware support

[📖 See detailed documentation](./flex_http_server/README.md)

### 🔗 [Grafana Webhook Integration](./webhooks/grafana-webhook/)
**Universal monitoring system integration bridge**

A webhook service that bridges Grafana Alertmanager to any FLEX paging HTTP server:
- **Integration**: Grafana Alertmanager → HackRF/TTGO FLEX paging servers
- **Compatibility**: Works with both HackRF and TTGO HTTP implementations
- **Features**: HTTPS support, configurable routing, comprehensive logging
- **Deployment**: Python-based service with systemd support
- **Use Case**: Automated alerting from monitoring systems to pagers via any backend

[📖 See detailed documentation](./webhooks/grafana-webhook/README.md)

## Quick Start

### Initial Setup

**⚠️ Important: Clone with submodules**

This project uses the tinyflex library as a submodule. You must clone recursively or initialize submodules manually:

```bash
# Option 1: Clone with submodules (recommended)
git clone --recursive https://github.com/geekinsanemx/flex-http-server.git

# Option 2: If already cloned, initialize submodules
git submodule update --init --recursive
```

### Choose Your Implementation

**For production/enterprise use with SDR hardware:**
```bash
cd hackrf_http_server/
make deps && make
./hackrf_http_server --verbose
```

**For cost-effective/portable solutions (original TTGO firmware):**
```bash
cd ttgo_http_server/
# First flash your TTGO device with ttgo-fsk-tx firmware
make && ./ttgo_http_server --verbose
```

**For modern AT command implementation (broader hardware support):**
```bash
cd flex_http_server/
# First flash your TTGO/Heltec device with flex-fsk-tx firmware
make && ./flex_http_server --verbose
```

**For Grafana monitoring integration with any backend:**
```bash
cd webhooks/grafana-webhook/
# Configure to point to either HackRF or TTGO server
sudo ./grafana-webhook.py --generate-config /etc/grafana-webhook/grafana-webhook.cfg
# Works with both http://localhost:16180 (HackRF or TTGO)
```

### Common API Usage

All HTTP implementations share a consistent API format:

```bash
# Send FLEX paging message
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{
    "capcode": 1122334,
    "message": "Hello World",
    "frequency": 931937500
  }'
```

## Hardware Requirements

### HackRF Implementation
- HackRF One or compatible SDR device
- Appropriate antenna for target frequency
- USB 3.0 connection recommended
- Linux system (Ubuntu/Debian preferred)

### TTGO Implementation (Original)
- TTGO LoRa32 V1/V2, TTGO T-Beam, TTGO LoRa32-OLED
- [ttgo-fsk-tx firmware](https://github.com/rlaneth/ttgo-fsk-tx/) (required dependency)
- USB connection for programming and communication
- Compatible antenna for chosen ISM band

### FLEX Implementation (AT Commands)
- TTGO LoRa32-OLED, Heltec WiFi LoRa 32 V3, Heltec LoRa32 V3
- [flex-fsk-tx firmware](https://github.com/geekinsanemx/flex-fsk-tx/) (required dependency)
- USB connection for programming and communication
- Compatible antenna for chosen ISM band

### Grafana Webhook
- Python 3.8+ environment
- Network access to either HackRF or TTGO HTTP server
- Grafana Alertmanager configuration

## Features Comparison

| Feature | HackRF Server | TTGO Server | FLEX Server | Grafana Webhook |
|---------|---------------|-------------|-------------|-----------------|
| **HTTP JSON API** | ✅ | ✅ | ✅ | ✅ (receives) |
| **TCP Serial Protocol** | ✅ | ✅ | ✅ | ❌ |
| **Authentication** | ✅ htpasswd | ✅ htpasswd | ✅ htpasswd | ✅ (to backend) |
| **Debug Mode** | ✅ | ✅ | ✅ | ✅ |
| **Verbose Logging** | ✅ | ✅ | ✅ | ✅ |
| **Systemd Service** | ✅ | ✅ | ✅ | ✅ |
| **AWS Lambda Compatible** | ✅ | ✅ | ✅ | ✅ |
| **EMR Support** | ✅ | ✅ | ✅ | via any backend |
| **Multi-band Support** | ✅ (VHF/UHF) | ✅ (ISM bands) | ✅ (ISM bands) | N/A |
| **Hardware Cost** | High (SDR) | Low (ESP32) | Low (ESP32) | Software only |
| **Communication Protocol** | Direct | Custom binary | AT commands | HTTP client |
| **Firmware Required** | None | ttgo-fsk-tx | flex-fsk-tx | N/A |
| **Backend Compatibility** | N/A | N/A | N/A | All servers |

## Common Configuration

All implementations support similar configuration patterns:

```ini
# Network Configuration
BIND_ADDRESS=127.0.0.1
HTTP_LISTEN_PORT=16180
SERIAL_LISTEN_PORT=16175

# Authentication
HTTP_AUTH_CREDENTIALS=passwords

# FLEX Protocol
DEFAULT_FREQUENCY=931937500
```

## Frequency Planning

### HackRF (VHF/UHF Paging Bands)
```
929.6625 MHz - American Messaging
931.9375 MHz - FLEX (default)
931.4375 MHz - Motorola infrastructure
```

### TTGO/FLEX (ISM Bands)
```
433.500 MHz - Global ISM band
868.000 MHz - Europe ISM band  
915.000 MHz - Americas ISM band
```

**⚠️ Always check local regulations and obtain proper licensing before transmitting**

## Integration Examples

### Python Client (Universal)
```python
import requests

def send_flex_message(server_url, username, password, capcode, message, frequency=None):
    payload = {
        'capcode': capcode,
        'message': message
    }
    if frequency:
        payload['frequency'] = frequency

    response = requests.post(
        f'{server_url}/',
        auth=(username, password),
        json=payload,
        headers={'Content-Type': 'application/json'}
    )
    return response.status_code == 200

# Works with HackRF, TTGO, and FLEX servers
send_flex_message('http://localhost:16180', 'admin', 'passw0rd',
                  1122334, 'Test message', 931937500)
```

### Monitoring Integration
```bash
# Grafana webhook → Any FLEX HTTP server
Grafana Alert → Webhook Service → HackRF/TTGO/FLEX Server → FLEX Transmission

# Configure webhook to use HackRF server
FLEX_SERVER_URL=http://localhost:16180  # HackRF server

# Or configure webhook to use TTGO server  
FLEX_SERVER_URL=http://localhost:16180  # TTGO server (original firmware)

# Or configure webhook to use FLEX server
FLEX_SERVER_URL=http://localhost:16180  # FLEX server (AT commands)

# Manual monitoring integration (works with all)
curl -X POST http://localhost:16180/ \
  -u admin:passw0rd \
  -H "Content-Type: application/json" \
  -d '{"capcode": 911911, "message": "Server down: web01"}'
```

## Contributing

Each implementation has its own build system and dependencies. See individual README files for specific development instructions:

- [HackRF development guide](./hackrf_http_server/README.md#troubleshooting)
- [TTGO development guide](./ttgo_http_server/README.md#development)  
- [FLEX development guide](./flex_http_server/README.md#development)
- [Webhook development](./webhooks/grafana-webhook/README.md)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

This work is based on and inspired by several excellent projects in the SDR and paging community:

* **[tinyflex](https://github.com/Theldus/tinyflex)** - A minimal FLEX paging transmitter that provided the foundation for the FLEX protocol implementation across all our servers
* **[ttgo-fsk-tx](https://github.com/rlaneth/ttgo-fsk-tx/)** - Original firmware for TTGO LoRa32 development boards that enables FSK transmission. **This is a required dependency for the TTGO implementation**
* **[flex-fsk-tx](https://github.com/geekinsanemx/flex-fsk-tx/)** - Modern AT command firmware for TTGO and Heltec development boards. **This is a required dependency for the FLEX implementation** and provides AT command interface for broader hardware compatibility
* **[ellisgl](https://github.com/ellisgl)** - For the hackrf_tcp_server contribution via [PR #6](https://github.com/Theldus/tinyflex/pull/6) to tinyflex, which helped shape the TCP server implementation and influenced our dual-protocol approach

Special thanks to these developers for their work in software-defined radio and paging protocols. The combination of traditional SDR hardware (HackRF), ESP32-based development boards with original firmware (TTGO with ttgo-fsk-tx), modern AT command implementation (TTGO/Heltec with flex-fsk-tx), and cloud integration (Grafana webhooks) provides users with flexible deployment options ranging from high-performance enterprise solutions to cost-effective IoT implementations.

## Support

For issues specific to each implementation:

1. **HackRF Server**: Check [HackRF troubleshooting guide](./hackrf_http_server/README.md#troubleshooting)
2. **TTGO Server**: Verify [ttgo-fsk-tx firmware installation](./ttgo_http_server/README.md#prerequisites)
3. **FLEX Server**: Verify [flex-fsk-tx firmware installation](./flex_http_server/README.md#prerequisites)
4. **Grafana Webhook**: Review [webhook configuration](./webhooks/grafana-webhook/README.md#troubleshooting) - works with all backends

For general repository issues:
- Check the [Issues](https://github.com/geekinsanemx/flex-http-server/issues) page
- Create a new issue with details about your specific implementation
- Include hardware setup, OS version, and configuration details

## Disclaimer

This software is intended for educational and experimental purposes. Users are responsible for ensuring compliance with local regulations regarding radio transmissions and paging frequencies. Always operate within legal power limits and obtain proper licensing when required.

## Related Projects

- [GNU Radio](https://www.gnuradio.org/) - The premier software toolkit for software radio
- [HackRF](https://github.com/mossmann/hackrf) - Low cost software defined radio platform
- [ttgo-fsk-tx](https://github.com/rlaneth/ttgo-fsk-tx/) - **Required firmware for TTGO implementation**
- [flex-fsk-tx](https://github.com/geekinsanemx/flex-fsk-tx/) - **Required firmware for FLEX implementation**
- [Universal Hardware Driver (UHD)](https://github.com/EttusResearch/uhd) - Hardware driver for USRP devices
