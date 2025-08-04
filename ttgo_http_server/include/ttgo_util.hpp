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

struct TtgoConfig {
    double frequency;  // Frequency in MHz
    int power;         // TX power 2-17
};

// Global for TTY restoration (similar to send_ttgo.c)
static struct termios orig_tty;
static bool tty_saved = false;
static int serial_fd = -1;

/**
 * @brief Configures the serial port with the specified baudrate.
 * Based on configure_serial from send_ttgo.c
 */
inline int configure_ttgo_serial(int fd, int baudrate) {
    struct termios tty;
    speed_t speed;

    if (tcgetattr(fd, &orig_tty) != 0) {
        perror("tcgetattr");
        return -1;
    }
    tty_saved = true;
    serial_fd = fd;
    tty = orig_tty;

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

    // TTY settings
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 10;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS; // no HW flow control
    tty.c_cflag |= CLOCAL | CREAD;

    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSIZE;   // Clear size bits
    tty.c_cflag |= CS8;      // 8 data bits
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // No software flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}

/**
 * @brief Restores original TTY settings if they were saved.
 */
inline void restore_ttgo_tty(void) {
    if (tty_saved && serial_fd >= 0) {
        tcsetattr(serial_fd, TCSANOW, &orig_tty);
        tty_saved = false;
    }
}

/**
 * @brief Reads from a given fd and discards all data sent
 * but not read until now. Based on discard_serial from send_ttgo.c
 */
inline int discard_ttgo_serial(int fd) {
    char dummy[1024];
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 2000) <= 0 || !(pfd.revents & POLLIN))
        return -1;
    return (int) read(fd, dummy, sizeof dummy);
}

/**
 * @brief Reads a full line from a given fd.
 * Based on read_serial_line from send_ttgo.c - EXACT COPY
 */
inline ssize_t read_ttgo_line(int fd, char *line, size_t line_size, bool verbose_mode = false) {
    ssize_t bytes_read;
    size_t total_read;

    tcflush(fd, TCIFLUSH);
    total_read = 0;
    line[line_size - 1] = '\0';

    if (verbose_mode) {
        std::cout << "  Reading TTGO response...\n";
    }

    while (total_read < line_size - 1) {
        bytes_read = read(fd, line + total_read, 1);
        if (bytes_read < 0) {
            perror("read");
            return -1;
        }
        if (bytes_read == 0) {
            if (verbose_mode) {
                std::cerr << "  Timeout reading TTGO response\n";
            }
            return -1;
        }
        if (line[total_read] == '\n') {
            line[total_read] = '\0';
            break;
        }
        total_read++;
    }
    return total_read;
}

/**
 * @brief Sends a given message to the file pointed by fd.
 * Based on send_serial from send_ttgo.c - EXACT COPY
 */
inline ssize_t send_ttgo_data(int fd, const char *msg, size_t size) {
    ssize_t bytes_written;
    size_t total_written;

    total_written = 0;
    size = (size == 0) ? strlen(msg) : size;

    while (total_written < size) {
        bytes_written = write(fd, msg + total_written, size - total_written);
        if (bytes_written < 0) {
            perror("write");
            return -1;
        }
        total_written += bytes_written;
    }
    tcdrain(fd);

    return 0;
}

/**
 * @brief Sends a command and checks for response via serial.
 * Based on send_command_and_check from send_ttgo.c - EXACT COPY with verbose option
 */
inline int send_ttgo_command_and_check(int fd, const char *command, int size,
    const char *error_msg, bool verbose_mode = false) {
    char response[256];

    if (verbose_mode) {
        std::cout << "  Sending TTGO command: (" << std::string(command, size - 1) << ")\n";
    }

    // Send command
    if (send_ttgo_data(fd, command, size) < 0)
        return -1;

    // Read response until newline, timeout or a matching line
    // NOTE: This is the EXACT logic from send_ttgo.c
    while (read_ttgo_line(fd, response, sizeof response, verbose_mode) >= 0) {
        if (strncmp(response, "CONSOLE:", 8) == 0) {
            // Check if error or not
            if (response[8] != '0') {
                if (verbose_mode) {
                    std::cerr << "  TTGO Error: (" << error_msg << "), Got: " << response << "\n";
                }
                return -1;
            }
            if (verbose_mode) {
                std::cout << "  TTGO Response: " << response << "\n";
            }
            break;
        }
    }

    return 0;
}

/**
 * @brief Sends binary data via serial and waits for response.
 * Based on send_binary_data from send_ttgo.c - EXACT COPY with verbose option
 */
inline int send_ttgo_binary_data(int fd, const uint8_t *data, size_t size, bool verbose_mode = false) {
    char response[256];

    if (verbose_mode) {
        std::cout << "  Sending binary data (" << size << " bytes)...\n";
    }

    // Send all binary data
    if (send_ttgo_data(fd, (const char*)data, size) < 0)
        return -1;

    // Read response until newline, timeout or a matching line
    // NOTE: This is the EXACT logic from send_ttgo.c
    while (read_ttgo_line(fd, response, sizeof response, verbose_mode) >= 0) {
        if (strncmp(response, "TX:", 3) == 0) {
            // Check if error or not
            if (response[3] != '0') {
                if (verbose_mode) {
                    std::cerr << "  TTGO TX Error: " << response << "\n";
                }
                return -1;
            }
            if (verbose_mode) {
                std::cout << "  TTGO TX Response: " << response << "\n";
            }
            break;
        }
    }

    return 0;
}

/**
 * @brief Opens and configures TTGO serial connection.
 */
inline int open_ttgo_serial(const std::string& device, int baudrate) {
    int fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        return -1;
    }

    if (configure_ttgo_serial(fd, baudrate) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief Closes TTGO serial connection and restores TTY.
 */
inline void close_ttgo_serial(int fd) {
    if (fd >= 0) {
        restore_ttgo_tty();
        close(fd);
    }
}

/**
 * @brief Sends a flex message via TTGO serial.
 * Based on send_flex_via_serial from send_ttgo.c - EXACT COPY with verbose option
 */
inline int send_flex_via_ttgo(int fd, const TtgoConfig& config,
    const uint8_t *data, size_t size, bool verbose_mode = false) {
    char cmd_buffer[64];
    int s;

    if (verbose_mode) {
        std::cout << "TTGO Transmission Details:\n";
        std::cout << "  Frequency: " << std::fixed << std::setprecision(4) << config.frequency << " MHz\n";
        std::cout << "  Power: " << config.power << "\n";
        std::cout << "  Data size: " << size << " bytes\n";
    }

    // Ignore all previous messages sent
    (void)discard_ttgo_serial(fd);

    // Set frequency - use EXACT same format as send_ttgo.c
    s = snprintf(cmd_buffer, sizeof(cmd_buffer), "f %.4f\n", config.frequency);
    if (send_ttgo_command_and_check(fd, cmd_buffer, s, "Failed to set frequency", verbose_mode) < 0)
        return -1;

    // Set TX power
    s = snprintf(cmd_buffer, sizeof(cmd_buffer), "p %d\n", config.power);
    if (send_ttgo_command_and_check(fd, cmd_buffer, s, "Failed to set TX power", verbose_mode) < 0)
        return -1;

    // Send message length
    s = snprintf(cmd_buffer, sizeof(cmd_buffer), "m %zu\n", size);
    if (send_ttgo_command_and_check(fd, cmd_buffer, s, "Failed to set message length", verbose_mode) < 0)
        return -1;

    // Send binary data
    if (send_ttgo_binary_data(fd, data, size, verbose_mode) < 0)
        return -1;

    if (verbose_mode) {
        std::cout << "  TTGO transmission completed successfully\n";
    }

    return 0;
}
