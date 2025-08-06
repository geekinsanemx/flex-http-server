#pragma once
#include <string>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <errno.h>
#include <iomanip>
#include <ctime>

struct FlexATConfig {
    double frequency;  // Frequency in MHz
    int power;         // TX power 2-20
};

// AT Protocol constants
#define AT_BUFFER_SIZE    1024
#define AT_TIMEOUT_MS     8000
#define AT_MAX_RETRIES    5
#define AT_INTER_CMD_DELAY 200
#define AT_DATA_SEND_TIMEOUT 20000

// Global for TTY restoration
static struct termios orig_at_tty;
static bool at_tty_saved = false;
static int at_serial_fd = -1;

// AT Protocol response types
typedef enum {
    AT_RESP_OK,
    AT_RESP_ERROR,
    AT_RESP_DATA,
    AT_RESP_TIMEOUT,
    AT_RESP_INVALID
} at_response_t;

/**
 * @brief Configures the serial port for AT commands communication.
 * Based on configure_serial from flex-fsk-tx.cpp
 */
inline int configure_flex_at_serial(int fd, int baudrate) {
    struct termios tty;
    speed_t speed;

    if (tcgetattr(fd, &orig_at_tty) != 0) {
        perror("tcgetattr");
        return -1;
    }
    at_tty_saved = true;
    at_serial_fd = fd;
    tty = orig_at_tty;

    // Convert baudrate to speed_t
    switch (baudrate) {
    case 9600:   speed = B9600;   break;
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
    case 230400: speed = B230400; break;
    default:
        fprintf(stderr, "Unsupported baudrate: %d\n", baudrate);
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    cfmakeraw(&tty);

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;  // 500ms timeout
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

/**
 * @brief Restores original TTY settings if they were saved.
 */
inline void restore_flex_at_tty(void) {
    if (at_tty_saved && at_serial_fd >= 0) {
        tcsetattr(at_serial_fd, TCSANOW, &orig_at_tty);
        at_tty_saved = false;
    }
}

/**
 * @brief Flush serial buffers completely.
 */
inline void flush_flex_at_buffers(int fd) {
    tcflush(fd, TCIOFLUSH);
    usleep(100000); // 100ms delay to ensure buffers are clear

    // Read any remaining data
    char dummy[256];
    int attempts = 0;
    while (attempts < 10) {
        ssize_t bytes = read(fd, dummy, sizeof(dummy));
        if (bytes <= 0) break;
        attempts++;
        usleep(10000); // 10ms between attempts
    }
}

/**
 * @brief Send AT command with proper flushing.
 */
inline int at_send_flex_command(int fd, const char *command, bool verbose_mode = false) {
    if (verbose_mode) {
        std::cout << "  Sending AT: " << std::string(command, strlen(command) - 2) << std::endl;
    }

    if (write(fd, command, strlen(command)) < 0) {
        perror("write");
        return -1;
    }
    tcdrain(fd);
    usleep(AT_INTER_CMD_DELAY * 1000); // Convert ms to us

    return 0;
}

/**
 * @brief Read AT response with improved parsing and timeout handling.
 * Based on at_read_response from flex-fsk-tx.cpp
 */
inline at_response_t at_read_flex_response(int fd, char *buffer, size_t buffer_size, bool verbose_mode = false) {
    char line_buffer[AT_BUFFER_SIZE];
    size_t line_pos = 0;
    int total_timeout = AT_TIMEOUT_MS;
    bool got_response = false;
    int empty_reads = 0;

    buffer[0] = '\0';

    while (total_timeout > 0 && empty_reads < 20) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;

        int poll_result = poll(&pfd, 1, 50); // Shorter poll interval

        if (poll_result < 0) {
            perror("poll");
            return AT_RESP_INVALID;
        }

        if (poll_result == 0) {
            total_timeout -= 50;
            empty_reads++;
            continue;
        }

        if (!(pfd.revents & POLLIN)) {
            total_timeout -= 50;
            continue;
        }

        char c;
        ssize_t bytes_read = read(fd, &c, 1);

        if (bytes_read < 0) {
            perror("read");
            return AT_RESP_INVALID;
        }

        if (bytes_read == 0) {
            total_timeout -= 50;
            empty_reads++;
            continue;
        }

        empty_reads = 0; // Reset empty read counter

        // Skip carriage returns
        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            // End of line
            line_buffer[line_pos] = '\0';

            // Skip empty lines
            if (line_pos == 0) {
                continue;
            }

            if (verbose_mode) {
                std::cout << "  Received AT: '" << line_buffer << "'" << std::endl;
            }

            // Check for responses
            if (strcmp(line_buffer, "OK") == 0) {
                return AT_RESP_OK;
            }
            else if (strcmp(line_buffer, "ERROR") == 0) {
                return AT_RESP_ERROR;
            }
            else if (strncmp(line_buffer, "+", 1) == 0) {
                // Data response
                if (strlen(line_buffer) < buffer_size) {
                    strcpy(buffer, line_buffer);
                    got_response = true;
                }
                // Continue reading to get OK/ERROR
            }
            else if (strstr(line_buffer, "DEBUG:") != NULL) {
                // Debug message from device
                if (verbose_mode) {
                    std::cout << "  Device debug: " << line_buffer << std::endl;
                }
            }
            else if (strstr(line_buffer, "AT READY") != NULL) {
                // Device ready message
                if (verbose_mode) {
                    std::cout << "  Device ready: " << line_buffer << std::endl;
                }
            }

            // Reset line buffer
            line_pos = 0;
        }
        else if (line_pos < sizeof(line_buffer) - 1 && c >= 32 && c <= 126) {
            // Printable character
            line_buffer[line_pos++] = c;
        }
        else if (c < 32 && c != '\r' && c != '\n') {
            // Non-printable character (except CR/LF) - reset line
            if (line_pos > 0) {
                if (verbose_mode) {
                    std::cout << "  Warning: Non-printable character 0x" << std::hex << (unsigned char)c
                              << std::dec << " in response, resetting line" << std::endl;
                }
                line_pos = 0;
            }
        }

        // Reset timeout on successful character read
        total_timeout = AT_TIMEOUT_MS;
    }

    if (got_response) {
        return AT_RESP_DATA;
    }

    return AT_RESP_TIMEOUT;
}

/**
 * @brief Send AT command and wait for response with enhanced retry logic.
 */
inline int at_execute_flex_command(int fd, const char *command, char *response,
                                   size_t response_size, bool verbose_mode = false) {
    int retries = AT_MAX_RETRIES;

    while (retries-- > 0) {
        // Clear buffers before sending command
        flush_flex_at_buffers(fd);

        if (at_send_flex_command(fd, command, verbose_mode) < 0) {
            return -1;
        }

        at_response_t result = at_read_flex_response(fd, response, response_size, verbose_mode);

        switch (result) {
        case AT_RESP_OK:
            return 0;
        case AT_RESP_ERROR:
            if (verbose_mode) {
                std::cerr << "  AT command failed: " << std::string(command, strlen(command) - 2) << std::endl;
            }
            if (retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Retrying command (" << retries << " attempts left)..." << std::endl;
                }
                usleep(500000); // 500ms delay between retries
                continue;
            }
            return -1;
        case AT_RESP_TIMEOUT:
            if (verbose_mode) {
                std::cerr << "  AT command timeout: " << std::string(command, strlen(command) - 2) << std::endl;
            }
            if (retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Retrying command due to timeout (" << retries << " attempts left)..." << std::endl;
                }
                // Send AT command to reset device state
                flush_flex_at_buffers(fd);
                at_send_flex_command(fd, "AT\r\n", verbose_mode);
                usleep(200000);
                at_read_flex_response(fd, response, response_size, verbose_mode);
                usleep(500000);
                continue;
            }
            return -1;
        case AT_RESP_INVALID:
            if (verbose_mode) {
                std::cerr << "  AT communication error: " << std::string(command, strlen(command) - 2) << std::endl;
            }
            if (retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Retrying command due to communication error (" << retries << " attempts left)..." << std::endl;
                }
                usleep(1000000); // 1 second delay for communication errors
                continue;
            }
            return -1;
        default:
            return -1;
        }
    }

    return -1;
}

/**
 * @brief Initialize device with AT commands and enhanced error recovery.
 */
inline int at_initialize_flex_device(int fd, bool verbose_mode = false) {
    char response[AT_BUFFER_SIZE];

    if (verbose_mode) {
        std::cout << "  Testing device communication..." << std::endl;
    }

    // Give device time to boot up
    flush_flex_at_buffers(fd);
    usleep(1000000); // 1 second

    // Try to establish communication
    for (int i = 0; i < 10; i++) {
        if (verbose_mode) {
            std::cout << "  Communication attempt " << (i + 1) << "/10..." << std::endl;
        }

        // Clear buffers thoroughly
        flush_flex_at_buffers(fd);
        usleep(200000); // 200ms

        // Send basic AT command
        if (at_execute_flex_command(fd, "AT\r\n", response, sizeof(response), verbose_mode) == 0) {
            if (verbose_mode) {
                std::cout << "  Device communication established" << std::endl;
            }

            // Send one more AT command to ensure stability
            usleep(200000);
            if (at_execute_flex_command(fd, "AT\r\n", response, sizeof(response), verbose_mode) == 0) {
                if (verbose_mode) {
                    std::cout << "  Device communication confirmed stable" << std::endl;
                }
                return 0;
            }
        }

        // Progressive delay between attempts
        usleep(500000 * (i + 1)); // 500ms, 1s, 1.5s, etc.
    }

    std::cerr << "Failed to establish communication after 10 attempts" << std::endl;
    return -1;
}

/**
 * @brief Opens and configures FLEX AT serial connection.
 */
inline int open_flex_at_serial(const std::string& device, int baudrate) {
    int fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        return -1;
    }

    if (configure_flex_at_serial(fd, baudrate) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief Closes FLEX AT serial connection and restores TTY.
 */
inline void close_flex_at_serial(int fd) {
    if (fd >= 0) {
        restore_flex_at_tty();
        close(fd);
    }
}

/**
 * @brief Sends a flex message via AT commands with enhanced retry logic.
 * Based on at_send_flex_message from flex-fsk-tx.cpp
 */
inline int send_flex_via_at_commands(int fd, const FlexATConfig& config,
                                    const uint8_t *data, size_t size, bool verbose_mode = false) {
    char command[128];
    char response[AT_BUFFER_SIZE];
    int send_retries = 3; // Retry sending the entire message up to 3 times

    if (verbose_mode) {
        std::cout << "AT Command Transmission Details:" << std::endl;
        std::cout << "  Frequency: " << std::fixed << std::setprecision(4) << config.frequency << " MHz" << std::endl;
        std::cout << "  Power: " << config.power << std::endl;
        std::cout << "  Data size: " << size << " bytes" << std::endl;
        std::cout << "  Configuring radio parameters..." << std::endl;
    }

    // Set frequency with retries
    snprintf(command, sizeof(command), "AT+FREQ=%.4f\r\n", config.frequency);
    if (at_execute_flex_command(fd, command, response, sizeof(response), verbose_mode) < 0) {
        std::cerr << "Failed to set frequency after all retries" << std::endl;
        return -1;
    }

    // Set power with retries
    snprintf(command, sizeof(command), "AT+POWER=%d\r\n", config.power);
    if (at_execute_flex_command(fd, command, response, sizeof(response), verbose_mode) < 0) {
        std::cerr << "Failed to set power after all retries" << std::endl;
        return -1;
    }

    if (verbose_mode) {
        std::cout << "  Radio configured successfully." << std::endl;
    }

    // Try to send the message with retries
    while (send_retries-- > 0) {
        if (verbose_mode) {
            std::cout << "  Attempting to send data (attempt " << (3 - send_retries) << "/3)..." << std::endl;
        }

        // Reset device state before attempting to send
        if (verbose_mode) {
            std::cout << "  Resetting device state..." << std::endl;
        }
        flush_flex_at_buffers(fd);
        if (at_execute_flex_command(fd, "AT\r\n", response, sizeof(response), verbose_mode) < 0) {
            if (verbose_mode) {
                std::cout << "  Failed to reset device state, continuing anyway..." << std::endl;
            }
        }

        // Send the SEND command
        snprintf(command, sizeof(command), "AT+SEND=%zu\r\n", size);
        if (verbose_mode) {
            std::cout << "  Sending command: " << std::string(command, strlen(command) - 2) << std::endl;
        }

        // Clear buffers before sending
        flush_flex_at_buffers(fd);

        if (write(fd, command, strlen(command)) < 0) {
            perror("write");
            if (send_retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Write failed, retrying..." << std::endl;
                }
                usleep(1000000);
                continue;
            }
            return -1;
        }
        tcdrain(fd);

        // Wait for device to be ready for data
        if (verbose_mode) {
            std::cout << "  Waiting for device to be ready for data..." << std::endl;
        }
        at_response_t result = at_read_flex_response(fd, response, sizeof(response), verbose_mode);

        if (result != AT_RESP_DATA || strstr(response, "+SEND: READY") == NULL) {
            if (verbose_mode) {
                std::cerr << "  Device not ready for data. Got response type " << result << ": '" << response << "'" << std::endl;
            }
            if (send_retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Device not ready, retrying entire send operation..." << std::endl;
                }
                usleep(2000000); // 2 second delay before retry
                continue;
            }
            return -1;
        }

        if (verbose_mode) {
            std::cout << "  Device ready! Sending " << size << " bytes of binary data..." << std::endl;
        }

        // Send binary data in smaller chunks with progress tracking
        size_t bytes_sent = 0;
        const size_t CHUNK_SIZE = 32; // Smaller chunks for more reliable transmission
        bool send_success = true;
        unsigned long send_start_time = time(NULL);

        while (bytes_sent < size && send_success) {
            size_t chunk_size = (size - bytes_sent > CHUNK_SIZE) ? CHUNK_SIZE : (size - bytes_sent);

            ssize_t written = write(fd, data + bytes_sent, chunk_size);
            if (written < 0) {
                perror("write binary data");
                send_success = false;
                break;
            }

            bytes_sent += written;
            if (verbose_mode) {
                std::cout << "  Sent " << bytes_sent << "/" << size << " bytes ("
                          << std::fixed << std::setprecision(1) << (float)bytes_sent * 100.0 / size << "%)" << std::endl;
            }

            // Check for timeout
            if ((time(NULL) - send_start_time) > (AT_DATA_SEND_TIMEOUT / 1000)) {
                if (verbose_mode) {
                    std::cout << "  Binary data send timeout" << std::endl;
                }
                send_success = false;
                break;
            }

            // Small delay between chunks to avoid overwhelming the device
            usleep(5000); // 5ms
        }

        if (!send_success) {
            if (send_retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Binary data send failed, retrying entire operation..." << std::endl;
                }
                usleep(2000000);
                continue;
            }
            return -1;
        }

        if (verbose_mode) {
            std::cout << "  Binary data sent successfully. Waiting for transmission completion..." << std::endl;
        }

        // Ensure all data is transmitted
        tcdrain(fd);
        sleep(5);

        // Wait for final response
        result = at_read_flex_response(fd, response, sizeof(response), verbose_mode);
        if (result != AT_RESP_OK) {
            if (verbose_mode) {
                std::cerr << "  Transmission failed. Response type " << result << ": '" << response << "'" << std::endl;
            }
            if (send_retries > 0) {
                if (verbose_mode) {
                    std::cout << "  Transmission failed, retrying entire operation..." << std::endl;
                }
                usleep(2000000);
                continue;
            }
            return -1;
        }

        if (verbose_mode) {
            std::cout << "  Transmission completed successfully!" << std::endl;
        }
        return 0;
    }

    std::cerr << "Failed to send message after all retry attempts" << std::endl;
    return -1;
}
