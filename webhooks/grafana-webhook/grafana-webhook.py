#!/usr/bin/env python3
"""
Grafana Webhook Service for FLEX Paging
========================================

This service receives Grafana alertmanager webhook notifications and forwards
them to any FLEX HTTP server (HackRF or TTGO) for paging transmission.

Features:
- Receives Grafana webhook alerts via REST API
- Parses multiple alerts from single JSON payload
- Extracts capcode and frequency from alert labels
- Prioritizes message content: summary > description > message
- HTTPS support with SSL certificates
- Comprehensive logging and error handling
- Systemd service integration
- Multiple configuration sources: CLI args > env vars > config file > defaults
- Universal compatibility with HackRF and TTGO HTTP servers

Author: Generated for FLEX Paging System
"""

import os
import sys
import json
import logging
import requests
import ssl
import argparse
import configparser
from datetime import datetime
from typing import List, Dict, Any, Optional, Tuple
from flask import Flask, request, jsonify
from werkzeug.serving import WSGIRequestHandler

# Configure basic logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger('grafana_webhook')

class GrafanaWebhook:
    def __init__(self, config_file: Optional[str] = None):
        """Initialize with configuration from multiple sources."""
        self.config = self.load_configuration(config_file)
        self.debug_mode = self.config.getboolean('MAIN', 'DEBUG_MODE', fallback=False)

        # Set logging level
        log_level = self.config.get('LOGGING', 'LOG_LEVEL', fallback='INFO').upper()
        logger.setLevel(getattr(logging, log_level, logging.INFO))

        if self.debug_mode:
            logger.setLevel(logging.DEBUG)
            logger.debug("Debug mode enabled")

        # Initialize Flask app
        self.app = Flask(__name__)
        self.setup_routes()

        # Determine port based on SSL configuration
        self.bind_port = self.config.getint('FLASK', 'BIND_PORT')
        if self.config.getboolean('SSL', 'SSL_ENABLED') and self.bind_port == 8080:
            self.bind_port = 8443  # Use 8443 for SSL by default

        logger.info("Grafana Webhook Service initialized")
        logger.info(f"FLEX Server: {self.config.get('FLEX', 'FLEX_SERVER_URL')}")
        logger.info(f"Bind Address: {self.config.get('FLASK', 'BIND_HOST')}:{self.bind_port}")
        logger.info(f"HTTPS Enabled: {self.config.getboolean('SSL', 'SSL_ENABLED')}")

    def load_configuration(self, config_file: Optional[str] = None) -> configparser.ConfigParser:
        """Load configuration from multiple sources with priority."""
        config = configparser.ConfigParser()

        # Set default values
        config.read_dict({
            'FLEX': {
                'FLEX_SERVER_URL': 'http://127.0.0.1:16180',
                'FLEX_USERNAME': 'admin',
                'FLEX_PASSWORD': 'passw0rd',
                'DEFAULT_CAPCODE': '',
                'DEFAULT_FREQUENCY': '931937500',
                'REQUEST_TIMEOUT': '30'
            },
            'FLASK': {
                'BIND_HOST': '0.0.0.0',
                'BIND_PORT': '8080'  # Default to 8080
            },
            'SSL': {
                'SSL_CERT_PATH': '',
                'SSL_KEY_PATH': '',
                'SSL_ENABLED': 'False'
            },
            'LOGGING': {
                'LOG_LEVEL': 'INFO'
            },
            'MAIN': {
                'DEBUG_MODE': 'False'
            }
        })

        # Load from config file if specified
        if config_file:
            if os.path.exists(config_file):
                logger.info(f"Loading configuration from {config_file}")
                config.read(config_file)
            else:
                logger.warning(f"Config file not found: {config_file}")

        # Override with environment variables
        for section in config.sections():
            for key in config[section]:
                env_var = os.getenv(key)
                if env_var is not None:
                    config.set(section, key, env_var)
                    logger.debug(f"Overriding config from ENV: {key}={env_var}")

        # Set SSL enabled flag
        ssl_enabled = (
            config.get('SSL', 'SSL_CERT_PATH') and
            config.get('SSL', 'SSL_KEY_PATH') and
            os.path.exists(config.get('SSL', 'SSL_CERT_PATH')) and
            os.path.exists(config.get('SSL', 'SSL_KEY_PATH'))
        )
        config.set('SSL', 'SSL_ENABLED', str(ssl_enabled))

        return config

    def setup_routes(self):
        """Setup Flask routes for the webhook service."""
        @self.app.route('/health', methods=['GET'])
        def health_check():
            return jsonify({
                'status': 'healthy',
                'timestamp': datetime.now().isoformat(),
                'service': 'grafana-webhook',
                'port': self.bind_port,
                'ssl': self.config.getboolean('SSL', 'SSL_ENABLED'),
                'flex_server': self.config.get('FLEX', 'FLEX_SERVER_URL')
            })

        @self.app.route('/api/v1/alerts', methods=['POST'])
        def webhook_handler():
            return self.handle_webhook()

        @self.app.route('/', methods=['POST'])
        def root_webhook_handler():
            return self.handle_webhook()

        @self.app.errorhandler(404)
        def not_found(error):
            return jsonify({'error': 'Endpoint not found'}), 404

        @self.app.errorhandler(500)
        def internal_error(error):
            logger.error(f"Internal server error: {error}")
            return jsonify({'error': 'Internal server error'}), 500

    def handle_webhook(self) -> Tuple[Dict[str, Any], int]:
        """Handle incoming Grafana webhook requests."""
        try:
            client_ip = request.environ.get('HTTP_X_FORWARDED_FOR', request.remote_addr)
            logger.info(f"Received webhook from {client_ip}")

            if self.debug_mode:
                logger.debug(f"Request headers: {dict(request.headers)}")

            if not request.is_json:
                logger.warning("Invalid content type, expected application/json")
                return {'error': 'Content-Type must be application/json'}, 400

            try:
                alerts_data = request.get_json()
            except Exception as e:
                logger.error(f"JSON parsing error: {e}")
                return {'error': 'Invalid JSON payload'}, 400

            if not isinstance(alerts_data, list):
                return {'error': 'Expected JSON array of alerts'}, 400

            logger.info(f"Processing {len(alerts_data)} alerts")
            results = []
            success_count = 0
            error_count = 0

            for i, alert in enumerate(alerts_data):
                try:
                    result = self.process_alert(alert, i + 1)
                    results.append(result)
                    success_count += 1 if result['success'] else 0
                    error_count += 0 if result['success'] else 1
                except Exception as e:
                    logger.error(f"Error processing alert {i + 1}: {e}")
                    results.append({
                        'alert_index': i + 1,
                        'success': False,
                        'error': str(e)
                    })
                    error_count += 1

            response = {
                'status': 'completed',
                'total_alerts': len(alerts_data),
                'successful': success_count,
                'failed': error_count,
                'results': results,
                'timestamp': datetime.now().isoformat()
            }

            logger.info(f"Processing complete: {success_count} successful, {error_count} failed")
            return response, 200 if error_count == 0 else 207

        except Exception as e:
            logger.error(f"Webhook handler error: {e}")
            return {'error': 'Internal processing error', 'details': str(e)}, 500

    def process_alert(self, alert: Dict[str, Any], alert_index: int) -> Dict[str, Any]:
        """Process a single Grafana alert."""
        try:
            labels = alert.get('labels', {})
            annotations = alert.get('annotations', {})

            # Determine alert status
            ends_at = alert.get('endsAt', '')
            status = "FIRING" if ends_at == "0001-01-01T00:00:00Z" else "RESOLVED"

            # Get alert name
            alert_name = labels.get('alertname', 'Unknown Alert')

            # Extract capcode
            capcode = self.config.get('FLEX', 'DEFAULT_CAPCODE')
            for key in ['capcode', 'pager_capcode', 'flex_capcode']:
                if key in labels:
                    try:
                        capcode = labels[key]
                        break
                    except (ValueError, TypeError):
                        pass
            # Keep capcode as string if it has leading zeros
            try:
                capcode = int(capcode)
            except ValueError:
                pass

            # Extract frequency
            frequency = self.config.getint('FLEX', 'DEFAULT_FREQUENCY')
            for key in ['frequency', 'pager_frequency', 'flex_frequency']:
                if key in labels:
                    try:
                        frequency = int(labels[key])
                        break
                    except (ValueError, TypeError):
                        pass

            # Get message content
            message_content = None
            for key in ['summary', 'description', 'message']:
                if key in annotations and annotations[key]:
                    message_content = str(annotations[key]).strip()
                    break
            if not message_content:
                message_content = "Alert triggered"

            final_message = f"[{status}] {alert_name}: {message_content}"

            # Prepare payload
            flex_payload = {
                'capcode': capcode,
                'message': final_message,
                'frequency': frequency
            }

            logger.info(f"Alert {alert_index}: Sending to FLEX server (capcode={capcode}, freq={frequency})")

            # Send to FLEX server
            success, response_data = self.send_to_flex_server(flex_payload)

            return {
                'alert_index': alert_index,
                'alert_name': alert_name,
                'capcode': capcode,
                'frequency': frequency,
                'success': success,
                'flex_response': response_data
            }

        except Exception as e:
            logger.error(f"Error processing alert {alert_index}: {e}")
            return {
                'alert_index': alert_index,
                'success': False,
                'error': str(e)
            }

    def send_to_flex_server(self, payload: Dict[str, Any]) -> Tuple[bool, Dict[str, Any]]:
        """Send message payload to FLEX HTTP server (HackRF or TTGO)."""
        try:
            url = self.config.get('FLEX', 'FLEX_SERVER_URL')
            auth = (
                self.config.get('FLEX', 'FLEX_USERNAME'),
                self.config.get('FLEX', 'FLEX_PASSWORD')
            )
            timeout = self.config.getint('FLEX', 'REQUEST_TIMEOUT')

            response = requests.post(
                url,
                auth=auth,
                json=payload,
                timeout=timeout,
                verify=True
            )

            if response.status_code == 200:
                return True, response.json()
            else:
                return False, {'status_code': response.status_code, 'response': response.text}

        except Exception as e:
            return False, {'error': str(e)}

    def run(self):
        """Run the Flask server."""
        ssl_context = None
        if self.config.getboolean('SSL', 'SSL_ENABLED'):
            ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
            ssl_context.load_cert_chain(
                self.config.get('SSL', 'SSL_CERT_PATH'),
                self.config.get('SSL', 'SSL_KEY_PATH')
            )
            logger.info(f"HTTPS enabled with SSL certificate")

        self.app.run(
            host=self.config.get('FLASK', 'BIND_HOST'),
            port=self.bind_port,
            ssl_context=ssl_context,
            threaded=True
        )

def generate_config(output_path: str):
    """Generate default configuration file."""
    config = configparser.ConfigParser()

    config['FLEX'] = {
        'FLEX_SERVER_URL': 'http://localhost:16180',
        'FLEX_USERNAME': 'admin',
        'FLEX_PASSWORD': 'passw0rd',
        'DEFAULT_CAPCODE': '0037137',
        'DEFAULT_FREQUENCY': '931937500',
        'REQUEST_TIMEOUT': '30'
    }

    config['FLASK'] = {
        'BIND_HOST': '0.0.0.0',
        'BIND_PORT': '8080'
    }

    config['SSL'] = {
        'SSL_CERT_PATH': '/etc/ssl/certs/grafana-webhook.crt',
        'SSL_KEY_PATH': '/etc/ssl/private/grafana-webhook.key'
    }

    config['LOGGING'] = {
        'LOG_LEVEL': 'INFO'
    }

    config['MAIN'] = {
        'DEBUG_MODE': 'False'
    }

    with open(output_path, 'w') as f:
        config.write(f)
    print(f"Generated default configuration at {output_path}")

def main():
    """Main entry point with command-line interface."""
    parser = argparse.ArgumentParser(
        description='Grafana Webhook Service for FLEX Paging',
        formatter_class=argparse.RawTextHelpFormatter,
        epilog='''Configuration Priority:
  1. Command-line arguments (--config, --verbose)
  2. Environment variables
  3. Configuration file
  4. Internal defaults

Environment Variables:
  FLEX_SERVER_URL      FLEX server URL (HackRF/TTGO)
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
  DEBUG_MODE           Enable debug mode (True/False)'''
    )

    parser.add_argument('-c', '--config', metavar='FILE',
                        help='Path to configuration file (default: /etc/grafana-webhook/grafana-webhook.cfg)')
    parser.add_argument('-g', '--generate-config', metavar='FILE',
                        help='Generate default configuration file')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Enable verbose/debug mode')
    args = parser.parse_args()

    # Handle config generation
    if args.generate_config:
        generate_config(args.generate_config)
        return

    # Set default config path if not specified
    if not args.config:
        default_cfg = '/etc/grafana-webhook/grafana-webhook.cfg'
        if os.path.exists(default_cfg):
            args.config = default_cfg

    # Initialize service
    service = GrafanaWebhook(config_file=args.config)

    # Override debug mode if requested
    if args.verbose:
        service.debug_mode = True
        logger.setLevel(logging.DEBUG)
        logger.debug("Verbose mode enabled via command line")

    # Run service
    service.run()

if __name__ == '__main__':
    main()
