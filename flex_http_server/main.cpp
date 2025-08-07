#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <sys/select.h>
#include <errno.h>
#include <iomanip>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include "../tinyflex/tinyflex.h"
#include "include/config.hpp"
#include "include/tcp_util.hpp"
#include "include/http_util.hpp"
#include "include/flex_at_util.hpp"

void print_help() {
    std::cout << "flex_http_server - FLEX paging HTTP/TCP server for FLEX-FSK-TX with AT commands\n";
    std::cout << "A dual-protocol server with AT command communication and comprehensive logging\n\n";

    std::cout << "USAGE:\n";
    std::cout << "  flex_http_server [OPTIONS]\n\n";

    std::cout << "OPTIONS:\n";
    std::cout << "  --help, -h     Show this help message and exit\n";
    std::cout << "  --debug, -d    Enable debug mode (show commands, skip transmission)\n";
    std::cout << "  --verbose, -v  Enable comprehensive pipeline logging (detailed processing info)\n\n";

    std::cout << "EXIT CODES (AWS Lambda Compatible):\n";
    std::cout << "  0  Success\n";
    std::cout << "  1  Invalid command line arguments\n";
    std::cout << "  2  Configuration errors\n";
    std::cout << "  3  Network setup errors (port binding)\n";
    std::cout << "  4  Authentication setup errors\n";
    std::cout << "  5  Serial device errors\n\n";

    std::cout << "CONFIGURATION:\n";
    std::cout << "  Reads config.ini (preferred) or environment variables as fallback.\n";
    std::cout << "  Both protocols can be independently enabled/disabled (set port to 0).\n\n";

    std::cout << "  Configuration parameters:\n";
    std::cout << "    BIND_ADDRESS        - IP address to bind to (default: 127.0.0.1)\n";
    std::cout << "    SERIAL_LISTEN_PORT  - TCP port for serial protocol (default: 16175, 0 = disabled)\n";
    std::cout << "    HTTP_LISTEN_PORT    - HTTP port for JSON API (default: 16180, 0 = disabled)\n";
    std::cout << "    HTTP_AUTH_CREDENTIALS - Password file path (default: passwords)\n";
    std::cout << "    FLEX_DEVICE         - Serial device path (default: /dev/ttyUSB0)\n";
    std::cout << "    FLEX_BAUDRATE       - Serial baudrate (default: 115200)\n";
    std::cout << "    FLEX_POWER          - TX power level (default: 2, range: 2-20)\n";
    std::cout << "    DEFAULT_FREQUENCY   - Default frequency Hz (default: 916000000)\n\n";

    std::cout << "FLEX-FSK-TX HARDWARE:\n";
    std::cout << "  This server communicates with devices running flex-fsk-tx firmware using AT commands.\n";
    std::cout << "  Supported devices: ESP32, Arduino, or any device with flex-fsk-tx AT command interface.\n";
    std::cout << "  Firmware repository: https://github.com/geekinsanemx/flex-fsk-tx/\n\n";

    std::cout << "AT COMMAND PROTOCOL:\n";
    std::cout << "  The server uses AT commands to communicate with the flex-fsk-tx device:\n";
    std::cout << "    AT                 - Test device communication\n";
    std::cout << "    AT+FREQ=<MHz>      - Set frequency (e.g., AT+FREQ=916.0000)\n";
    std::cout << "    AT+POWER=<level>   - Set TX power 2-20 (e.g., AT+POWER=10)\n";
    std::cout << "    AT+SEND=<bytes>    - Prepare to send binary data\n";
    std::cout << "    <binary data>      - Send FLEX message binary data\n\n";

    std::cout << "SERIAL PROTOCOL (TCP) - Legacy Support:\n";
    std::cout << "  Format: {CAPCODE}|{MESSAGE}|{FREQUENCY_HZ}\n";
    std::cout << "  Example: echo '001122334|Hello World|916000000' | nc localhost 16175\n\n";

    std::cout << "HTTP PROTOCOL (JSON API) - Modern REST API:\n";
    std::cout << "  Endpoint: POST http://localhost:16180/\n";
    std::cout << "  Authentication: HTTP Basic Auth (required)\n";
    std::cout << "  Content-Type: application/json\n\n";

    std::cout << "  JSON Format (capcode and message are REQUIRED, frequency is optional):\n";
    std::cout << "  {\n";
    std::cout << "    \"capcode\": 1122334,      // REQUIRED: target capcode\n";
    std::cout << "    \"message\": \"Hello World\", // REQUIRED: message text\n";
    std::cout << "    \"frequency\": 916000000   // OPTIONAL: uses DEFAULT_FREQUENCY if omitted\n";
    std::cout << "  }\n\n";

    std::cout << "  HTTP Response Codes (AWS Lambda Compatible):\n";
    std::cout << "    200 OK                - Message transmitted successfully\n";
    std::cout << "    400 Bad Request       - Invalid JSON or missing required fields\n";
    std::cout << "    401 Unauthorized      - Authentication required/failed\n";
    std::cout << "    405 Method Not Allowed - Only POST requests supported\n";
    std::cout << "    500 Internal Error    - Processing/transmission failure\n\n";

    std::cout << "EXAMPLES:\n";
    std::cout << "  # Send message via HTTP\n";
    std::cout << "  curl -X POST http://localhost:16180/ -u admin:passw0rd \\\n";
    std::cout << "    -H 'Content-Type: application/json' \\\n";
    std::cout << "    -d '{\"capcode\":1122334,\"message\":\"Test Message\",\"frequency\":916000000}'\n\n";

    std::cout << "  # Send message via TCP\n";
    std::cout << "  echo '1122334|Test Message|916000000' | nc localhost 16175\n\n";

    std::cout << "DEBUGGING:\n";
    std::cout << "  --verbose: Shows all AT communication and FLEX encoding details\n";
    std::cout << "  --debug:   Shows commands but skips actual transmission\n\n";
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

void send_emr_messages(int flex_fd, const FlexATConfig& config, bool debug_mode, bool verbose_mode) {
    if (verbose_mode) {
        std::cout << "EMR Transmission:" << std::endl;
        std::cout << "  Status: STARTING EMR (Emergency Message Resynchronization)..." << std::endl;
    }

    if (debug_mode) {
        if (verbose_mode) {
            std::cout << "  Status: SKIPPED (debug mode active)" << std::endl << std::endl;
        }
        return;
    }

    // EMR message is typically a short synchronization burst
    uint8_t emr_buffer[] = {0xA5, 0x5A, 0xA5, 0x5A}; // Simple sync pattern

    if (send_flex_via_at_commands(flex_fd, config, emr_buffer, sizeof(emr_buffer), verbose_mode) == 0) {
        if (verbose_mode) {
            std::cout << "  Status: EMR COMPLETED" << std::endl << std::endl;
        }
    } else {
        if (verbose_mode) {
            std::cout << "  Status: EMR FAILED" << std::endl << std::endl;
        }
    }
}

void log_message_processing_start(uint64_t capcode, const std::string& message, uint64_t frequency, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "=== Message Processing Started ===" << std::endl;
    std::cout << "Input Parameters:" << std::endl;
    std::cout << "  CAPCODE: " << capcode << std::endl;
    std::cout << "  MESSAGE: '" << message << "' (" << message.length() << " characters)" << std::endl;
    std::cout << "  FREQUENCY: " << frequency << " Hz (" << std::fixed << std::setprecision(6)
              << (frequency / 1000000.0) << " MHz)" << std::endl << std::endl;
}

void log_capcode_validation(uint64_t capcode, bool verbose_mode) {
    if (!verbose_mode) return;

    int is_long;
    bool valid = is_capcode_valid(capcode, &is_long);

    std::cout << "Capcode Validation:" << std::endl;
    std::cout << "  Capcode: " << capcode << " is " << (is_long ? "LONG (32-bit)" : "SHORT (18-bit)") << std::endl;
    std::cout << "  Status: " << (valid ? "VALID" : "INVALID") << std::endl << std::endl;
}

void log_flex_encoding(const uint8_t* flex_buffer, size_t flex_len, const std::string& message, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "FLEX Encoding:" << std::endl;
    std::cout << "  Input message length: " << message.length() << " bytes" << std::endl;
    std::cout << "  Buffer size: 1024 bytes" << std::endl;
    std::cout << "  Encoded length: " << flex_len << " bytes" << std::endl;
    std::cout << "  Encoding status: SUCCESS" << std::endl;
    std::cout << "  Encoded FLEX data: ";

    // Print ALL bytes in hex format
    for (size_t i = 0; i < flex_len; ++i) {
        if (i > 0 && i % 16 == 0) {
            std::cout << std::endl << "                     ";
        }
        std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
                  << static_cast<int>(flex_buffer[i]) << " ";
    }
    std::cout << std::dec << std::endl << std::endl;
}

void log_flex_at_setup(uint64_t frequency, int power, const std::string& device, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "FLEX AT Setup:" << std::endl;
    std::cout << "  Serial device: " << device << std::endl;
    std::cout << "  Target frequency: " << frequency << " Hz (" << std::fixed << std::setprecision(6)
              << (frequency / 1000000.0) << " MHz)" << std::endl;
    std::cout << "  TX power: " << power << std::endl;
    std::cout << "  FLEX device: READY" << std::endl << std::endl;
}

void log_flex_transmission_start(bool debug_mode, bool verbose_mode) {
    if (!verbose_mode) return;

    std::cout << "FLEX AT Transmission:" << std::endl;
    if (debug_mode) {
        std::cout << "  Status: SKIPPED (debug mode active)" << std::endl;
    } else {
        std::cout << "  Status: STARTING..." << std::endl;
    }
}

void log_flex_transmission_complete(bool debug_mode, bool verbose_mode) {
    if (!verbose_mode) return;

    if (!debug_mode) {
        std::cout << "  Status: COMPLETED" << std::endl;
    }
    std::cout << "=== Message Processing Completed ===" << std::endl << std::endl;
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
    struct tf_message_config msg_config = {0};
    msg_config.mail_drop = 0; // Can be configured if needed

    size_t flex_len = tf_encode_flex_message_ex(message.c_str(), capcode, flex_buffer,
                                               sizeof(flex_buffer), &error, &msg_config);

    if (error < 0) {
        std::cerr << "Error encoding message: " << error << std::endl;
        return false;
    }
    log_flex_encoding(flex_buffer, flex_len, message, verbose_mode);

    // Setup FLEX AT connection
    int flex_fd = open_flex_at_serial(config.FLEX_DEVICE, config.FLEX_BAUDRATE);
    if (flex_fd < 0) {
        std::cerr << "Failed to open FLEX device: " << config.FLEX_DEVICE << std::endl;
        return false;
    }
    log_flex_at_setup(frequency, config.FLEX_POWER, config.FLEX_DEVICE, verbose_mode);

    // Initialize AT communication
    if (at_initialize_flex_device(flex_fd, verbose_mode) < 0) {
        std::cerr << "Failed to initialize FLEX device" << std::endl;
        close_flex_at_serial(flex_fd);
        return false;
    }

    // Create FLEX AT config for transmission
    FlexATConfig flex_config;
    flex_config.frequency = frequency / 1000000.0; // Convert Hz to MHz
    flex_config.power = config.FLEX_POWER;

    // Check if we need to send EMR messages
    bool need_emr = should_send_emr(conn_state);
    if (need_emr) {
        send_emr_messages(flex_fd, flex_config, debug_mode, verbose_mode);
    }

    // Transmit FLEX message via AT commands
    log_flex_transmission_start(debug_mode, verbose_mode);
    bool success = false;
    if (!debug_mode) {
        success = (send_flex_via_at_commands(flex_fd, flex_config, flex_buffer, flex_len, verbose_mode) == 0);

        if (success) {
            // Update connection state
            conn_state.last_transmission = std::chrono::steady_clock::now();
            conn_state.first_message = false;
        }
    } else {
        success = true; // In debug mode, we consider it successful
    }
    log_flex_transmission_complete(debug_mode, verbose_mode);

    close_flex_at_serial(flex_fd);
    return success;
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
    char buffer[8192] = {0};
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
            std::cout << "Failed to read initial HTTP data from client" << std::endl;
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
        std::cout << std::endl << "=== HTTP Client Connected ===" << std::endl;
        std::cout << "Client IP: " << client_ip << std::endl;
        std::cout << "Client Port: " << client_port << std::endl;
        std::cout << "Initial read: " << initial_read << " bytes" << std::endl;
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
                        std::cout << "Found Content-Length: " << content_length << std::endl;
                    }
                } catch (...) {
                    if (verbose_mode) {
                        std::cout << "Failed to parse Content-Length: '" << length_str << "'" << std::endl;
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
            std::cout << "Body bytes received so far: " << body_received << std::endl;
            std::cout << "Expected body length: " << content_length << std::endl;
        }

        // Read more data if needed
        while (body_received < content_length) {
            int additional_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (additional_read <= 0) {
                if (verbose_mode) {
                    std::cout << "Failed to read additional body data" << std::endl;
                }
                break;
            }

            buffer[additional_read] = '\0';
            full_request += std::string(buffer, additional_read);
            body_received += additional_read;
            total_read += additional_read;

            if (verbose_mode) {
                std::cout << "Read additional " << additional_read << " bytes" << std::endl;
                std::cout << "Total body received: " << body_received << "/" << content_length << std::endl;
            }
        }
    }

    if (verbose_mode) {
        std::cout << "Final HTTP Request (" << full_request.length() << " bytes):" << std::endl;
        std::cout << "---" << std::endl << full_request << "---" << std::endl;
    }

    HttpRequest request = parse_http_request(full_request);
    log_parsed_request(request, verbose_mode);

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
            std::cout << "*** JSON MESSAGE PARSING FAILED ***" << std::endl;
            std::cout << "Body was: '" << request.body << "'" << std::endl;
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
            std::cout << "HTTP client disconnected." << std::endl << std::endl;
        }
    } else {
        send_http_response(client_fd, 500, "Internal Server Error",
                          "{\"error\":\"Failed to process message\",\"code\":500}",
                          "application/json", verbose_mode);
    }
}

// Signal handler for graceful shutdown
static volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    (void)sig;
    keep_running = 0;
    printf("\nShutdown signal received, stopping server...\n");
}

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

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
            std::cout << "Configuration loaded from config.ini" << std::endl;
        }
    } else {
        // Fall back to environment variables
        if (verbose_mode) {
            std::cout << "config.ini not found, using environment variables" << std::endl;
        }

        const char* env_bind = getenv("BIND_ADDRESS");
        const char* env_serial_port = getenv("SERIAL_LISTEN_PORT");
        const char* env_http_port = getenv("HTTP_LISTEN_PORT");
        const char* env_auth_credentials = getenv("HTTP_AUTH_CREDENTIALS");
        const char* env_flex_device = getenv("FLEX_DEVICE");
        const char* env_flex_baudrate = getenv("FLEX_BAUDRATE");
        const char* env_flex_power = getenv("FLEX_POWER");
        const char* env_default_freq = getenv("DEFAULT_FREQUENCY");

        // Set defaults
        config.BIND_ADDRESS = env_bind ? std::string(env_bind) : "127.0.0.1";
        config.SERIAL_LISTEN_PORT = env_serial_port ? std::stoul(env_serial_port) : 16175;
        config.HTTP_LISTEN_PORT = env_http_port ? std::stoul(env_http_port) : 16180;
        config.HTTP_AUTH_CREDENTIALS = env_auth_credentials ? std::string(env_auth_credentials) : "passwords";
        config.FLEX_DEVICE = env_flex_device ? std::string(env_flex_device) : "/dev/ttyUSB0";
        config.FLEX_BAUDRATE = env_flex_baudrate ? std::stoul(env_flex_baudrate) : 115200;
        config.FLEX_POWER = env_flex_power ? std::stoi(env_flex_power) : 2;
        config.DEFAULT_FREQUENCY = env_default_freq ? std::stoull(env_default_freq) : 916000000;

        config_loaded = true;
    }

    if (!config_loaded) {
        std::cerr << "Failed to load configuration!" << std::endl;
        return 2;
    }

    // Validate FLEX configuration
    if (config.FLEX_POWER < 2 || config.FLEX_POWER > 20) {
        std::cerr << "Invalid FLEX_POWER: " << config.FLEX_POWER << " (must be 2-20)" << std::endl;
        return 2;
    }

    if (verbose_mode) {
        std::cout << "Configuration:" << std::endl;
        std::cout << "  BIND_ADDRESS: " << config.BIND_ADDRESS << std::endl;
        std::cout << "  SERIAL_LISTEN_PORT: " << config.SERIAL_LISTEN_PORT << std::endl;
        std::cout << "  HTTP_LISTEN_PORT: " << config.HTTP_LISTEN_PORT << std::endl;
        std::cout << "  HTTP_AUTH_CREDENTIALS: " << config.HTTP_AUTH_CREDENTIALS << std::endl;
        std::cout << "  FLEX_DEVICE: " << config.FLEX_DEVICE << std::endl;
        std::cout << "  FLEX_BAUDRATE: " << config.FLEX_BAUDRATE << std::endl;
        std::cout << "  FLEX_POWER: " << config.FLEX_POWER << std::endl;
        std::cout << "  DEFAULT_FREQUENCY: " << config.DEFAULT_FREQUENCY << std::endl;
    }

    // Check if both ports are disabled
    if (config.SERIAL_LISTEN_PORT == 0 && config.HTTP_LISTEN_PORT == 0) {
        std::cerr << "Error: Both SERIAL_LISTEN_PORT and HTTP_LISTEN_PORT are disabled (set to 0)!" << std::endl;
        std::cerr << "At least one port must be enabled." << std::endl;
        return 2;
    }

    // Test FLEX connection if not in debug mode
    if (!debug_mode) {
        int test_fd = open_flex_at_serial(config.FLEX_DEVICE, config.FLEX_BAUDRATE);
        if (test_fd < 0) {
            std::cerr << "Failed to open FLEX device: " << config.FLEX_DEVICE << std::endl;
            std::cerr << "Check device path and permissions, or use --debug mode for testing." << std::endl;
            return 5;
        }

        // Test AT communication
        if (at_initialize_flex_device(test_fd, verbose_mode) < 0) {
            std::cerr << "Failed to initialize FLEX device with AT commands" << std::endl;
            std::cerr << "Verify that flex-fsk-tx firmware is loaded and AT commands are working." << std::endl;
            close_flex_at_serial(test_fd);
            return 5;
        }

        close_flex_at_serial(test_fd);
        if (verbose_mode) {
            std::cout << "FLEX device connection and AT commands test: SUCCESS" << std::endl;
        }
    }

    // Setup servers
    int serial_server_fd = -1;
    int http_server_fd = -1;
    struct sockaddr_in serial_address, http_address;

    if (config.SERIAL_LISTEN_PORT > 0) {
        serial_server_fd = setup_tcp_server(config.SERIAL_LISTEN_PORT, serial_address, config.BIND_ADDRESS);
        if (serial_server_fd < 0) {
            std::cerr << "Failed to setup serial TCP server" << std::endl;
            return 3;
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
            return 3;
        }
        printf("HTTP server listening on %s:%d\n", config.BIND_ADDRESS.c_str(), config.HTTP_LISTEN_PORT);
    } else {
        printf("HTTP server disabled (port = 0)\n");
    }

    // Load or create passwords file for HTTP authentication
    std::map<std::string, std::string> passwords;
    if (config.HTTP_LISTEN_PORT > 0) {
        passwords = load_passwords(config.HTTP_AUTH_CREDENTIALS);
        if (passwords.empty()) {
            std::cout << "Passwords file not found at '" << config.HTTP_AUTH_CREDENTIALS
                      << "', creating default one..." << std::endl;
            if (create_default_passwords(config.HTTP_AUTH_CREDENTIALS)) {
                passwords = load_passwords(config.HTTP_AUTH_CREDENTIALS);
            } else {
                std::cerr << "Failed to create default passwords file at '"
                          << config.HTTP_AUTH_CREDENTIALS << "'!" << std::endl;
                if (serial_server_fd >= 0) close(serial_server_fd);
                if (http_server_fd >= 0) close(http_server_fd);
                return 4;
            }
        }
        if (verbose_mode) {
            std::cout << "Loaded " << passwords.size() << " user(s) from '"
                      << config.HTTP_AUTH_CREDENTIALS << "'" << std::endl;
        }
    }

    ConnectionState conn_state;
    printf("FLEX HTTP/TCP Server ready, waiting for connections...\n");
    printf("Press Ctrl+C to stop the server gracefully.\n");

    // Main server loop using select() with proper signal handling
    while (keep_running) {
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

        // Shorter timeout for more responsive shutdown
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; // 500ms instead of 1 second

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        // Check keep_running immediately after select returns
        if (!keep_running) {
            break;
        }

        if (activity < 0) {
            if (errno == EINTR) {
                // Signal interrupted select - check keep_running and continue
                continue;
            } else {
                perror("select error");
                break;
            }
        }

        if (activity == 0) {
            // Timeout - just continue to check keep_running
            continue;
        }

        // Handle serial TCP connections
        if (serial_server_fd >= 0 && FD_ISSET(serial_server_fd, &read_fds)) {
            socklen_t serial_addrlen = sizeof(serial_address);
            int client_fd = accept(serial_server_fd, (struct sockaddr *)&serial_address, &serial_addrlen);
            if (client_fd >= 0) {
                if (verbose_mode) {
                    std::cout << "Serial TCP client connected from " << inet_ntoa(serial_address.sin_addr) << std::endl;
                } else {
                    printf("Serial TCP client connected!\n");
                }

                handle_serial_client(client_fd, conn_state, config, debug_mode, verbose_mode);
                close(client_fd);

                if (verbose_mode) {
                    std::cout << "Serial TCP client connection closed." << std::endl;
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
    printf("\nShutting down servers...\n");
    if (serial_server_fd >= 0) {
        close(serial_server_fd);
        printf("Serial TCP server stopped.\n");
    }
    if (http_server_fd >= 0) {
        close(http_server_fd);
        printf("HTTP server stopped.\n");
    }

    printf("FLEX HTTP/TCP Server stopped gracefully.\n");
    return 0;
}
