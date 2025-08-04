#include <cstring>
#include <iostream>
#include <libhackrf/hackrf.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <sys/select.h>
#include <errno.h>
#include <iomanip>
#include <arpa/inet.h>
#include "../../tinyflex.h"
#include "include/config.hpp"
#include "include/fsk.hpp"
#include "include/hackrf_util.hpp"
#include "include/flex_util.hpp"
#include "include/tcp_util.hpp"
#include "include/http_util.hpp"
#include "include/iq_util.hpp"

#ifndef M_TAU
// Why calculate 2 * PI when we can just use a constant?
#define M_TAU 6.28318530717958647692
#endif

void print_help() {
    std::cout << "hackrf_http_server - FLEX paging HTTP/TCP server for HackRF\n";
    std::cout << "A dual-protocol server with comprehensive logging and AWS Lambda compatible response codes\n\n";

    std::cout << "USAGE:\n";
    std::cout << "  hackrf_http_server [OPTIONS]\n\n";

    std::cout << "OPTIONS:\n";
    std::cout << "  --help, -h     Show this help message and exit\n";
    std::cout << "  --debug, -d    Enable debug mode (hex dumps, IQ file output, skip transmission)\n";
    std::cout << "  --verbose, -v  Enable comprehensive pipeline logging (detailed processing info)\n\n";

    std::cout << "EXIT CODES (AWS Lambda Compatible):\n";
    std::cout << "  0  Success\n";
    std::cout << "  1  Invalid command line arguments\n";
    std::cout << "  2  Configuration errors\n";
    std::cout << "  3  Network setup errors (port binding)\n";
    std::cout << "  4  Authentication setup errors\n\n";

    std::cout << "CONFIGURATION:\n";
    std::cout << "  Reads config.ini (preferred) or environment variables as fallback.\n";
    std::cout << "  Both protocols can be independently enabled/disabled (set port to 0).\n\n";

    std::cout << "  Configuration parameters:\n";
    std::cout << "    BIND_ADDRESS        - IP address to bind to (default: 127.0.0.1)\n";
    std::cout << "    SERIAL_LISTEN_PORT  - TCP port for serial protocol (default: 16175, 0 = disabled)\n";
    std::cout << "    HTTP_LISTEN_PORT    - HTTP port for JSON API (default: 16180, 0 = disabled)\n";
    std::cout << "    HTTP_AUTH_CREDENTIALS - Password file path (default: passwords)\n"; // NEW
    std::cout << "    SAMPLE_RATE         - HackRF sample rate (default: 2000000, min: 2M)\n";
    std::cout << "    BITRATE             - FSK bitrate (default: 1600, min for 2FSK Flex)\n";
    std::cout << "    AMPLITUDE           - Signal amplitude (default: 127, range: -127 to 127)\n";
    std::cout << "    FREQ_DEV            - Frequency deviation Hz (default: 2400, ±2400Hz = 4800Hz total)\n";
    std::cout << "    TX_GAIN             - HackRF TX gain dB (default: 0, range: 0-47)\n";
    std::cout << "    DEFAULT_FREQUENCY   - Default frequency Hz (default: 931937500)\n\n";

    std::cout << "SERIAL PROTOCOL (TCP) - Legacy Support:\n";
    std::cout << "  Format: {CAPCODE}|{MESSAGE}|{FREQUENCY_HZ}\n";
    std::cout << "  Example: echo '001122334|Hello World|925516000' | nc localhost 16175\n\n";

    std::cout << "HTTP PROTOCOL (JSON API) - Modern REST API:\n";
    std::cout << "  Endpoint: POST http://localhost:16180/\n";
    std::cout << "  Authentication: HTTP Basic Auth (required)\n";
    std::cout << "  Content-Type: application/json\n\n";

    std::cout << "  JSON Format (capcode and message are REQUIRED, frequency is optional):\n";
    std::cout << "  {\n";
    std::cout << "    \"capcode\": 1122334,      // REQUIRED: target capcode\n";
    std::cout << "    \"message\": \"Hello World\", // REQUIRED: message text\n";
    std::cout << "    \"frequency\": 925516000   // OPTIONAL: uses DEFAULT_FREQUENCY if omitted\n";
    std::cout << "  }\n\n";

    std::cout << "  HTTP Response Codes (AWS Lambda Compatible):\n";
    std::cout << "    200 OK                - Message transmitted successfully\n";
    std::cout << "    400 Bad Request       - Invalid JSON or missing required fields (capcode/message)\n";
    std::cout << "    401 Unauthorized      - Authentication required/failed\n";
    std::cout << "    405 Method Not Allowed - Only POST requests supported\n";
    std::cout << "    500 Internal Error    - Processing/transmission failure\n\n";

    std::cout << "  Examples:\n";
    std::cout << "    # Full message with all parameters\n";
    std::cout << "    curl -X POST http://localhost:16180/ -u admin:passw0rd \\\n";
    std::cout << "      -H 'Content-Type: application/json' \\\n";
    std::cout << "      -d '{\"capcode\":1122334,\"message\":\"Test\",\"frequency\":925516000}'\n\n";
    std::cout << "    # Required fields only (uses DEFAULT_FREQUENCY)\n";
    std::cout << "    curl -X POST http://localhost:16180/ -u admin:passw0rd \\\n";
    std::cout << "      -H 'Content-Type: application/json' \\\n";
    std::cout << "      -d '{\"capcode\":1122334,\"message\":\"Using default frequency\"}'\n\n";

    std::cout << "AUTHENTICATION:\n";
    std::cout << "  HTTP requests require basic auth. Credentials file specified by HTTP_AUTH_CREDENTIALS.\n";
    std::cout << "  Default: ./passwords (htpasswd format)\n";
    std::cout << "  Auto-created with admin/passw0rd if file missing\n\n";

    std::cout << "  User management:\n";
    std::cout << "    htpasswd -B <password_file> username    # Add/update (bcrypt, recommended)\n";
    std::cout << "    htpasswd -m <password_file> username    # Add/update (MD5, compatible)\n";
    std::cout << "    htpasswd -D <password_file> username    # Delete user\n";
    std::cout << "    htpasswd -v <password_file> username    # Verify password\n\n";

    std::cout << "VERBOSE LOGGING:\n";
    std::cout << "  Use --verbose for comprehensive pipeline visibility:\n";
    std::cout << "  • HTTP client connection details (IP, port, raw request)\n";
    std::cout << "  • Request parsing and JSON processing\n";
    std::cout << "  • Message processing pipeline with validation\n";
    std::cout << "  • FLEX encoding with hex dumps\n";
    std::cout << "  • HackRF device setup and configuration\n";
    std::cout << "  • FSK modulation parameters and sample generation\n";
    std::cout << "  • RF transmission progress and completion\n";
    std::cout << "  • HTTP response codes and body content\n\n";

    std::cout << "DEBUG MODE:\n";
    std::cout << "  Use --debug for signal analysis without transmission:\n";
    std::cout << "  • Creates flexserver_output.iq file for GNU Radio analysis\n";
    std::cout << "  • Shows raw FLEX encoding in hex format\n";
    std::cout << "  • Displays EMR status without actual EMR transmission\n";
    std::cout << "  • Safe for testing without HackRF device\n\n";

    std::cout << "EMR (Emergency Message Resynchronization):\n";
    std::cout << "  Automatic synchronization for reliable paging:\n";
    std::cout << "  • Sends EMR before first message or after 10+ minute gaps\n";
    std::cout << "  • Ensures proper receiver synchronization\n";
    std::cout << "  • EMR transmission logged in verbose mode\n\n";

    std::cout << "ADVANCED FEATURES:\n";
    std::cout << "  • Capcode validation (SHORT 18-bit / LONG 32-bit auto-detection)\n";
    std::cout << "  • Independent protocol enable/disable (set port to 0)\n";
    std::cout << "  • Comprehensive error handling with detailed logging\n";
    std::cout << "  • Cloud service integration ready (standard HTTP codes)\n";
    std::cout << "  • Real-time transmission monitoring\n\n";

    std::cout << "For detailed documentation, examples, and troubleshooting:\n";
    std::cout << "See README.md or visit the project repository.\n\n";
}

struct ConnectionState {
    std::chrono::steady_clock::time_point last_transmission;
    bool first_message;

    ConnectionState() : first_message(true) {}
};

bool should_send_emr(ConnectionState& state) {
    if (state.first_message) {
        return true;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(
        now - state.last_transmission);

    return duration.count() >= 10;
}

void send_emr_messages(hackrf_device* device, const Config& config, bool verbose_mode) {
    if (verbose_mode) {
        std::cout << "EMR Transmission:\n";
        std::cout << "  Status: STARTING EMR (Emergency Message Resynchronization)...\n";
    }

    // EMR message is typically a short synchronization burst
    // Using a standard EMR pattern for FLEX
    uint8_t emr_buffer[] = {0xA5, 0x5A, 0xA5, 0x5A}; // Simple sync pattern

    std::vector<int8_t> emr_iq_samples = generate_fsk_iq_samples(
        emr_buffer,
        sizeof(emr_buffer),
        config.SAMPLE_RATE,
        config.BITRATE,
        config.AMPLITUDE,
        config.FREQ_DEV
    );

    if (verbose_mode) {
        std::cout << "  EMR IQ samples: " << emr_iq_samples.size() / 2 << " pairs\n";
        std::cout << "  Status: TRANSMITTING EMR...\n";
    }

    transmit_hackrf(device, emr_iq_samples);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (verbose_mode) {
        std::cout << "  Status: EMR COMPLETED\n";
    }
}

void log_message_processing_start(uint64_t capcode, const std::string& message, uint64_t frequency, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "=== Message Processing Started ===\n";
    std::cout << "Input Parameters:\n";
    std::cout << "  CAPCODE: " << capcode << "\n";
    std::cout << "  MESSAGE: '" << message << "' (" << message.length() << " characters)\n";
    std::cout << "  FREQUENCY: " << frequency << " Hz (" << std::fixed << std::setprecision(6)
              << (frequency / 1000000.0) << " MHz)\n\n";
}

void log_capcode_validation(uint64_t capcode, bool verbose_mode) {
    if (!verbose_mode) return;

    int is_long;
    bool valid = is_capcode_valid(capcode, &is_long);

    std::cout << "Capcode Validation:\n";
    std::cout << "  Capcode: " << capcode << " is " << (is_long ? "LONG (32-bit)" : "SHORT (18-bit)") << "\n";
    std::cout << "  Status: " << (valid ? "VALID" : "INVALID") << "\n\n";
}

void log_flex_encoding(const uint8_t* flex_buffer, size_t flex_len, const std::string& message, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "FLEX Encoding:\n";
    std::cout << "  Input message length: " << message.length() << " bytes\n";
    std::cout << "  Buffer size: 1024 bytes\n";
    std::cout << "  Encoded length: " << flex_len << " bytes\n";
    std::cout << "  Encoding status: SUCCESS\n";
    std::cout << "  Encoded FLEX data: ";

    // Print ALL bytes in hex format (removed the 24-line limit)
    for (size_t i = 0; i < flex_len; ++i) {
        if (i > 0 && i % 16 == 0) {
            std::cout << "\n                     ";
        }
        std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
                  << static_cast<int>(flex_buffer[i]) << " ";
    }
    std::cout << std::dec << "\n\n";
}

void log_binary_analysis(size_t flex_len, int bitrate, bool verbose_mode) {
    if (!verbose_mode) return;

    size_t total_bits = flex_len * 8;
    double transmission_time = (double)total_bits / bitrate * 1000; // in ms

    std::cout << "Binary Analysis:\n";
    std::cout << "  Total bits to transmit: " << total_bits << "\n";
    std::cout << "  Estimated transmission time: " << std::fixed << std::setprecision(2)
              << transmission_time << " ms\n\n";
}

void log_hackrf_setup(uint64_t frequency, uint32_t sample_rate, uint8_t tx_gain, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "HackRF Setup:\n";
    std::cout << "  Target frequency: " << frequency << " Hz (" << std::fixed << std::setprecision(6)
              << (frequency / 1000000.0) << " MHz)\n";
    std::cout << "  Sample rate: " << sample_rate << " Hz (" << (sample_rate / 1000000.0) << " MSPS)\n";
    std::cout << "  TX gain: " << static_cast<int>(tx_gain) << " dB\n";
    std::cout << "  HackRF device: READY\n\n";
}

void log_fsk_modulation(const std::vector<int8_t>& iq_samples, const Config& config, bool verbose_mode) {
    if (!verbose_mode) return;

    double samples_per_bit = (double)config.SAMPLE_RATE / config.BITRATE;
    double sample_duration = (iq_samples.size() / 2.0) / config.SAMPLE_RATE * 1000; // in ms

    std::cout << "FSK Modulation:\n";
    std::cout << "  Bitrate: " << config.BITRATE << " bps\n";
    std::cout << "  Samples per bit: " << std::fixed << std::setprecision(2) << samples_per_bit << "\n";
    std::cout << "  Frequency deviation: ±" << config.FREQ_DEV << " Hz\n";
    std::cout << "  Amplitude: " << static_cast<int>(config.AMPLITUDE) << " ("
              << std::setprecision(1) << (static_cast<int>(config.AMPLITUDE) / 127.0 * 100) << "%)\n";
    std::cout << "  Generated IQ samples: " << iq_samples.size() << " (" << (iq_samples.size() / 2) << " I/Q pairs)\n";
    std::cout << "  Sample duration: " << std::setprecision(2) << sample_duration << " ms\n";

    // Show first 10 I/Q pairs
    std::cout << "  First 10 I/Q pairs: ";
    for (size_t i = 0; i < std::min(size_t(20), iq_samples.size()); i += 2) {
        std::cout << "(" << static_cast<int>(iq_samples[i]) << "," << static_cast<int>(iq_samples[i+1]) << ") ";
        if (i >= 18) break;
    }
    std::cout << "\n\n";
}

void log_file_output(const std::string& filename, size_t sample_count, bool debug_mode, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "File Output:\n";
    if (debug_mode) {
        std::cout << "Wrote " << sample_count << " IQ samples to " << filename << "\n";
        std::cout << "  IQ file: SUCCESS (" << filename << ")\n\n";
    } else {
        std::cout << "  IQ file: SKIPPED (not in debug mode)\n\n";
    }
}

void log_rf_transmission_start(bool debug_mode, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "RF Transmission:\n";
    if (debug_mode) {
        std::cout << "  Status: SKIPPED (debug mode active)\n";
    } else {
        std::cout << "  Status: STARTING...\n";
    }
}

void log_rf_transmission_complete(bool debug_mode, bool verbose_mode) {
    if (!verbose_mode) return;

    if (!debug_mode) {
        std::cout << "  Status: COMPLETED\n";
    }
    std::cout << "=== Message Processing Completed ===\n\n";
}

bool process_message(uint64_t capcode, const std::string& message, uint64_t frequency,
                    ConnectionState& conn_state, const Config& config,
                    bool debug_mode, bool verbose_mode) {

    log_message_processing_start(capcode, message, frequency, verbose_mode);

    // Validate capcode
    int is_long;
    if (!is_capcode_valid(capcode, &is_long)) {
        std::cerr << "Invalid capcode: " << capcode << std::endl;
        return false;
    }
    log_capcode_validation(capcode, verbose_mode);

    // Validate frequency
    if (frequency < 1000000 || frequency > 6000000000) {
        std::cerr << "Frequency out of valid range: " << frequency << std::endl;
        return false;
    }

    // Encode message using TinyFlex
    uint8_t flex_buffer[1024];
    int error = 0;
    size_t flex_len = 0;
    if (!encode_flex_message(message, capcode, flex_buffer, sizeof(flex_buffer), flex_len, error)) {
        std::cerr << "Error encoding message: " << error << std::endl;
        return false;
    }
    log_flex_encoding(flex_buffer, flex_len, message, verbose_mode);
    log_binary_analysis(flex_len, config.BITRATE, verbose_mode);

    // --- HackRF transmitter setup ---
    hackrf_device* device = setup_hackrf(frequency, config.SAMPLE_RATE, config.TX_GAIN);
    if (!device) {
        return false;
    }
    log_hackrf_setup(frequency, config.SAMPLE_RATE, config.TX_GAIN, verbose_mode);

    // Check if we need to send EMR messages
    bool need_emr = should_send_emr(conn_state);
    if (need_emr && !debug_mode) {
        send_emr_messages(device, config, verbose_mode);
    } else if (need_emr && debug_mode) {
        if (verbose_mode) {
            std::cout << "EMR Transmission:\n";
            std::cout << "  Status: SKIPPED (debug mode active)\n\n";
        }
    }

    // Generate FSK IQ samples from FLEX buffer
    std::vector<int8_t> iq_samples = generate_fsk_iq_samples(
        flex_buffer,
        flex_len,
        config.SAMPLE_RATE,
        config.BITRATE,
        config.AMPLITUDE,
        config.FREQ_DEV
    );
    log_fsk_modulation(iq_samples, config, verbose_mode);

    // --- Write IQ samples to file for analysis (debug mode) ---
    if (debug_mode) {
        write_iq_file("flexserver_output.iq", iq_samples);
    }
    log_file_output("flexserver_output.iq", iq_samples.size() / 2, debug_mode, verbose_mode);

    // --- Transmit IQ samples ---
    log_rf_transmission_start(debug_mode, verbose_mode);
    if (!debug_mode) {
        transmit_hackrf(device, iq_samples);

        // Update connection state
        conn_state.last_transmission = std::chrono::steady_clock::now();
        conn_state.first_message = false;
    }
    log_rf_transmission_complete(debug_mode, verbose_mode);

    close_hackrf(device);
    return true;
}

void handle_serial_client(int client_fd, ConnectionState& conn_state, const Config& config,
                         bool debug_mode, bool verbose_mode) {
    char buffer[2048] = {0};

    // Read input from client
    int valRead = read(client_fd, buffer, sizeof(buffer) - 1);
    if (valRead <= 0) {
        perror("read");
        return;
    }

    buffer[valRead] = '\0';

    // Parse input: {CAPCODE}|{MESSAGE}|{FREQUENCY IN HZ}
    std::string input(buffer);
    size_t pos1 = input.find('|');
    size_t pos2 = input.rfind('|');
    if (pos1 == std::string::npos || pos2 == std::string::npos || pos1 == pos2) {
        std::string error_msg = "Invalid input format. Expected: CAPCODE|MESSAGE|FREQUENCY";
        send(client_fd, error_msg.c_str(), error_msg.size(), 0);
        return;
    }

    std::string capcode_str = input.substr(0, pos1);
    std::string message     = input.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string freq_str    = input.substr(pos2 + 1);

    uint64_t capcode;
    uint64_t frequency;

    try {
        capcode = std::stoull(capcode_str);
        frequency = std::stoull(freq_str);
    } catch (const std::exception& e) {
        std::string error_msg = "Invalid capcode or frequency format";
        send(client_fd, error_msg.c_str(), error_msg.size(), 0);
        return;
    }

    if (process_message(capcode, message, frequency, conn_state, config, debug_mode, verbose_mode)) {
        std::string success_msg = "Message sent successfully!";
        send(client_fd, success_msg.c_str(), success_msg.size(), 0);
    } else {
        std::string error_msg = "Failed to process message";
        send(client_fd, error_msg.c_str(), error_msg.size(), 0);
    }
}

void handle_http_client(int client_fd, const std::map<std::string, std::string>& passwords,
                       ConnectionState& conn_state, const Config& config,
                       bool debug_mode, bool verbose_mode) {
    char buffer[8192] = {0}; // Increased buffer size
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Get client info for logging
    std::string client_ip = "unknown";
    int client_port = 0;
    if (getpeername(client_fd, (struct sockaddr*)&client_addr, &client_len) == 0) {
        client_ip = inet_ntoa(client_addr.sin_addr);
        client_port = ntohs(client_addr.sin_port);
    }

    // Enhanced HTTP request reading with proper handling of body
    std::string full_request;
    int total_read = 0;
    int content_length = 0;
    bool headers_complete = false;
    size_t headers_end_pos = 0;

    // Read initial chunk
    int initial_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (initial_read <= 0) {
        if (verbose_mode) {
            std::cout << "Failed to read initial HTTP data from client\n";
        }
        send_http_response(client_fd, 400, "Bad Request",
                          "{\"error\":\"Failed to read request\",\"code\":400}",
                          "application/json", verbose_mode);
        return;
    }

    buffer[initial_read] = '\0';
    full_request = std::string(buffer, initial_read);
    total_read = initial_read;

    if (verbose_mode) {
        std::cout << "\n=== HTTP Client Connected ===\n";
        std::cout << "Client IP: " << client_ip << "\n";
        std::cout << "Client Port: " << client_port << "\n";
        std::cout << "Initial read: " << initial_read << " bytes\n";
        std::cout << "Buffer content: '" << std::string(buffer, initial_read) << "'\n";
    }

    // Check if headers are complete (look for \r\n\r\n)
    headers_end_pos = full_request.find("\r\n\r\n");
    if (headers_end_pos != std::string::npos) {
        headers_complete = true;
        headers_end_pos += 4; // Include the \r\n\r\n
    }

    // Parse headers to get Content-Length
    std::istringstream header_stream(full_request);
    std::string line;
    while (std::getline(header_stream, line) && !line.empty() && line != "\r") {
        line.erase(line.find_last_not_of("\r\n") + 1); // Remove trailing \r\n
        if (line.empty()) break;

        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        if (line.find("content-length:") == 0) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string length_str = line.substr(colon_pos + 1);
                length_str.erase(0, length_str.find_first_not_of(" \t"));
                try {
                    content_length = std::stoi(length_str);
                    if (verbose_mode) {
                        std::cout << "Found Content-Length: " << content_length << "\n";
                    }
                } catch (...) {
                    if (verbose_mode) {
                        std::cout << "Failed to parse Content-Length: '" << length_str << "'\n";
                    }
                }
            }
            break;
        }
    }

    // If we have content-length, make sure we read the complete body
    if (content_length > 0 && headers_complete) {
        size_t body_start = headers_end_pos;
        int body_received = full_request.length() - body_start;

        if (verbose_mode) {
            std::cout << "Headers end at position: " << headers_end_pos << "\n";
            std::cout << "Body start at position: " << body_start << "\n";
            std::cout << "Body bytes received so far: " << body_received << "\n";
            std::cout << "Expected body length: " << content_length << "\n";
        }

        // Read more data if needed
        while (body_received < content_length) {
            int additional_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (additional_read <= 0) {
                if (verbose_mode) {
                    std::cout << "Failed to read additional body data\n";
                }
                break;
            }

            buffer[additional_read] = '\0';
            full_request += std::string(buffer, additional_read);
            body_received += additional_read;
            total_read += additional_read;

            if (verbose_mode) {
                std::cout << "Read additional " << additional_read << " bytes\n";
                std::cout << "Total body received: " << body_received << "/" << content_length << "\n";
            }
        }
    }

    if (verbose_mode) {
        std::cout << "Final HTTP Request (" << full_request.length() << " bytes):\n";
        std::cout << "---\n" << full_request << "---\n";

        // Analyze the request structure
        std::cout << "Request Analysis:\n";
        std::cout << "  Total length: " << full_request.length() << " bytes\n";
        std::cout << "  Contains \\r\\n\\r\\n: " << (full_request.find("\r\n\r\n") != std::string::npos ? "YES" : "NO") << "\n";
        std::cout << "  Headers end position: " << headers_end_pos << "\n";

        if (headers_end_pos < full_request.length()) {
            std::string body_part = full_request.substr(headers_end_pos);
            std::cout << "  Body part length: " << body_part.length() << " bytes\n";
            std::cout << "  Body content: '" << body_part << "'\n";
        } else {
            std::cout << "  Body part: NONE FOUND\n";
        }
    }

    HttpRequest request = parse_http_request(full_request);
    log_parsed_request(request, verbose_mode);

    // Additional diagnostics for empty body
    if (request.body.empty() && content_length > 0) {
        if (verbose_mode) {
            std::cout << "*** WARNING: Body is empty but Content-Length is " << content_length << " ***\n";
            std::cout << "*** This indicates an HTTP parsing issue ***\n";

            // Try to manually extract body
            size_t manual_headers_end = full_request.find("\r\n\r\n");
            if (manual_headers_end != std::string::npos) {
                std::string manual_body = full_request.substr(manual_headers_end + 4);
                std::cout << "Manual body extraction: '" << manual_body << "'\n";
                std::cout << "Manual body length: " << manual_body.length() << "\n";

                // Override the parsed body if manual extraction worked
                if (!manual_body.empty() && request.body.empty()) {
                    request.body = manual_body;
                    std::cout << "*** Using manually extracted body ***\n";
                }
            }
        }
    }

    // Check if it's a POST request
    if (request.method != "POST") {
        send_http_response(client_fd, 405, "Method Not Allowed",
                          "{\"error\":\"Only POST method is allowed\",\"code\":405}",
                          "application/json", verbose_mode);
        return;
    }

    // Check authentication
    auto auth_it = request.headers.find("authorization");
    if (auth_it == request.headers.end() || !authenticate_user(auth_it->second, passwords)) {
        send_unauthorized_response(client_fd, verbose_mode);
        return;
    }

    // Parse JSON message
    JsonMessage json_msg = parse_json_message(request.body);
    if (!json_msg.valid) {
        if (verbose_mode) {
            std::cout << "*** JSON MESSAGE PARSING FAILED ***\n";
            std::cout << "Body was: '" << request.body << "'\n";
            std::cout << "Body length: " << request.body.length() << "\n";
        }
        send_http_response(client_fd, 400, "Bad Request",
                          "{\"error\":\"Invalid JSON format or missing required fields\",\"code\":400}",
                          "application/json", verbose_mode);
        return;
    }

    // Validate required fields: capcode and message are MANDATORY
    if (json_msg.capcode == 0) {
        send_http_response(client_fd, 400, "Bad Request",
                          "{\"error\":\"Missing required field: capcode must be specified\",\"code\":400}",
                          "application/json", verbose_mode);
        return;
    }

    if (json_msg.message.empty()) {
        send_http_response(client_fd, 400, "Bad Request",
                          "{\"error\":\"Missing required field: message must be specified\",\"code\":400}",
                          "application/json", verbose_mode);
        return;
    }

    log_json_processing(json_msg, config.DEFAULT_FREQUENCY, verbose_mode);

    // Use default frequency if not provided (frequency is optional)
    uint64_t frequency = json_msg.frequency > 0 ? json_msg.frequency : config.DEFAULT_FREQUENCY;

    if (process_message(json_msg.capcode, json_msg.message, frequency, conn_state, config, debug_mode, verbose_mode)) {
        send_http_response(client_fd, 200, "OK",
                          "{\"status\":\"success\",\"message\":\"Message transmitted successfully\"}",
                          "application/json", verbose_mode);

        if (verbose_mode) {
            std::cout << "HTTP client disconnected.\n\n";
        }
    } else {
        send_http_response(client_fd, 500, "Internal Server Error",
                          "{\"error\":\"Failed to process message\",\"code\":500}",
                          "application/json", verbose_mode);
    }
}

int main(int argc, char* argv[]) {
    // Parse CLI arguments
    bool debug_mode = false;
    bool verbose_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--debug" || arg == "-d") {
            debug_mode = true;
        } else if (arg == "--verbose" || arg == "-v") {
            verbose_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }

    Config config;
    bool config_loaded = false;

    // Try to load config.ini first
    if (load_config("config.ini", config)) {
        config_loaded = true;
        if (verbose_mode) {
            std::cout << "Configuration loaded from config.ini\n";
        }
    } else {
        // Fall back to environment variables
        if (verbose_mode) {
            std::cout << "config.ini not found, using environment variables\n";
        }

        const char* env_bind = getenv("BIND_ADDRESS");
        const char* env_serial_port = getenv("SERIAL_LISTEN_PORT");
        const char* env_http_port = getenv("HTTP_LISTEN_PORT");
        const char* env_port = getenv("PORT"); // Legacy support
        const char* env_auth_credentials = getenv("HTTP_AUTH_CREDENTIALS"); // NEW
        const char* env_sample_rate = getenv("SAMPLE_RATE");
        const char* env_bitrate = getenv("BITRATE");
        const char* env_amplitude = getenv("AMPLITUDE");
        const char* env_freq_dev = getenv("FREQ_DEV");
        const char* env_tx_gain = getenv("TX_GAIN");
        const char* env_default_freq = getenv("DEFAULT_FREQUENCY");

        // Set defaults
        config.BIND_ADDRESS = env_bind ? std::string(env_bind) : "127.0.0.1";
        config.SERIAL_LISTEN_PORT = env_serial_port ? std::stoul(env_serial_port) :
                                   (env_port ? std::stoul(env_port) : 16175);
        config.HTTP_LISTEN_PORT = env_http_port ? std::stoul(env_http_port) : 16180;
        config.HTTP_AUTH_CREDENTIALS = env_auth_credentials ? std::string(env_auth_credentials) : "passwords"; // NEW
        config.SAMPLE_RATE = env_sample_rate ? std::stoull(env_sample_rate) : 2000000;
        config.BITRATE = env_bitrate ? std::stoul(env_bitrate) : 1600;
        config.AMPLITUDE = env_amplitude ? static_cast<int8_t>(std::stoi(env_amplitude)) : 127;
        config.FREQ_DEV = env_freq_dev ? std::stoul(env_freq_dev) : 2400;
        config.TX_GAIN = env_tx_gain ? static_cast<uint8_t>(std::stoi(env_tx_gain)) : 0;
        config.DEFAULT_FREQUENCY = env_default_freq ? std::stoull(env_default_freq) : 931937500;

        config_loaded = true;
    }

    if (!config_loaded) {
        std::cerr << "Failed to load configuration!" << std::endl;
        return 2;
    }

    if (verbose_mode) {
        std::cout << "Configuration:\n";
        std::cout << "  BIND_ADDRESS: " << config.BIND_ADDRESS << "\n";
        std::cout << "  SERIAL_LISTEN_PORT: " << config.SERIAL_LISTEN_PORT << "\n";
        std::cout << "  HTTP_LISTEN_PORT: " << config.HTTP_LISTEN_PORT << "\n";
        std::cout << "  HTTP_AUTH_CREDENTIALS: " << config.HTTP_AUTH_CREDENTIALS << "\n";
        std::cout << "  SAMPLE_RATE: " << config.SAMPLE_RATE << "\n";
        std::cout << "  BITRATE: " << config.BITRATE << "\n";
        std::cout << "  AMPLITUDE: " << static_cast<int>(config.AMPLITUDE) << "\n";
        std::cout << "  FREQ_DEV: " << config.FREQ_DEV << "\n";
        std::cout << "  TX_GAIN: " << static_cast<int>(config.TX_GAIN) << "\n";
        std::cout << "  DEFAULT_FREQUENCY: " << config.DEFAULT_FREQUENCY << "\n";
    }

    // Check if both ports are disabled
    if (config.SERIAL_LISTEN_PORT == 0 && config.HTTP_LISTEN_PORT == 0) {
        std::cerr << "Error: Both SERIAL_LISTEN_PORT and HTTP_LISTEN_PORT are disabled (set to 0)!" << std::endl;
        std::cerr << "At least one port must be enabled." << std::endl;
        return 2; // Use exit code 2 for configuration errors
    }

    // Setup servers
    int serial_server_fd = -1;
    int http_server_fd = -1;
    struct sockaddr_in serial_address, http_address;

    if (config.SERIAL_LISTEN_PORT > 0) {
        serial_server_fd = setup_tcp_server(config.SERIAL_LISTEN_PORT, serial_address, config.BIND_ADDRESS);
        if (serial_server_fd < 0) {
            std::cerr << "Failed to setup serial TCP server" << std::endl;
            return 3; // Use exit code 3 for network setup errors
        }
        printf("Serial TCP server listening on %s:%d\n", config.BIND_ADDRESS.c_str(), config.SERIAL_LISTEN_PORT);
    } else {
        printf("Serial TCP server disabled (port = 0)\n");
    }

    if (config.HTTP_LISTEN_PORT > 0) {
        http_server_fd = setup_tcp_server(config.HTTP_LISTEN_PORT, http_address, config.BIND_ADDRESS);
        if (http_server_fd < 0) {
            if (serial_server_fd >= 0) close(serial_server_fd);
            std::cerr << "Failed to setup HTTP server" << std::endl;
            return 3; // Use exit code 3 for network setup errors
        }
        printf("HTTP server listening on %s:%d\n", config.BIND_ADDRESS.c_str(), config.HTTP_LISTEN_PORT);
    } else {
        printf("HTTP server disabled (port = 0)\n");
    }

    // Load or create passwords file for HTTP authentication
    std::map<std::string, std::string> passwords;
    if (config.HTTP_LISTEN_PORT > 0) {
        passwords = load_passwords(config.HTTP_AUTH_CREDENTIALS); // CHANGED: use config value
        if (passwords.empty()) {
            std::cout << "Passwords file not found at '" << config.HTTP_AUTH_CREDENTIALS
                      << "', creating default one..." << std::endl;
            if (create_default_passwords(config.HTTP_AUTH_CREDENTIALS)) { // CHANGED: use config value
                passwords = load_passwords(config.HTTP_AUTH_CREDENTIALS); // CHANGED: use config value
            } else {
                std::cerr << "Failed to create default passwords file at '"
                          << config.HTTP_AUTH_CREDENTIALS << "'!" << std::endl;
                if (serial_server_fd >= 0) close(serial_server_fd);
                if (http_server_fd >= 0) close(http_server_fd);
                return 4; // Use exit code 4 for authentication setup errors
            }
        }
        if (verbose_mode) {
            std::cout << "Loaded " << passwords.size() << " user(s) from '"
                      << config.HTTP_AUTH_CREDENTIALS << "'\n";
        }
    }

    ConnectionState conn_state;
    printf("Server ready, waiting for connections...\n");

    // Main server loop using select()
    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        int max_fd = 0;
        if (serial_server_fd >= 0) {
            FD_SET(serial_server_fd, &read_fds);
            max_fd = std::max(max_fd, serial_server_fd);
        }
        if (http_server_fd >= 0) {
            FD_SET(http_server_fd, &read_fds);
            max_fd = std::max(max_fd, http_server_fd);
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        // Handle serial TCP connections
        if (serial_server_fd >= 0 && FD_ISSET(serial_server_fd, &read_fds)) {
            socklen_t serial_addrlen = sizeof(serial_address);
            int client_fd = accept(serial_server_fd, (struct sockaddr *)&serial_address, &serial_addrlen);
            if (client_fd >= 0) {
                if (verbose_mode) {
                    std::cout << "Serial TCP client connected from " << inet_ntoa(serial_address.sin_addr) << "\n";
                } else {
                    printf("Serial TCP client connected!\n");
                }

                handle_serial_client(client_fd, conn_state, config, debug_mode, verbose_mode);
                close(client_fd);

                if (verbose_mode) {
                    std::cout << "Serial TCP client connection closed.\n";
                }
            }
        }

        // Handle HTTP connections
        if (http_server_fd >= 0 && FD_ISSET(http_server_fd, &read_fds)) {
            socklen_t http_addrlen = sizeof(http_address);
            int client_fd = accept(http_server_fd, (struct sockaddr *)&http_address, &http_addrlen);
            if (client_fd >= 0) {
                if (!verbose_mode) {
                    printf("HTTP client connected!\n");
                }

                handle_http_client(client_fd, passwords, conn_state, config, debug_mode, verbose_mode);
                close(client_fd);
            }
        }
    }

    // Cleanup
    if (serial_server_fd >= 0) close(serial_server_fd);
    if (http_server_fd >= 0) close(http_server_fd);
    return 0;
}
