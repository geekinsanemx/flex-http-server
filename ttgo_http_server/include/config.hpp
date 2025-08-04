#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>

struct Config {
    std::string BIND_ADDRESS;
    uint32_t SERIAL_LISTEN_PORT;
    uint32_t HTTP_LISTEN_PORT;
    std::string HTTP_AUTH_CREDENTIALS;

    // TTGO-specific configuration
    std::string TTGO_DEVICE;
    uint32_t TTGO_BAUDRATE;
    int TTGO_POWER;
    uint64_t DEFAULT_FREQUENCY;
};

// Helper function to trim whitespace and trailing commas
inline std::string trim_config_value(const std::string& str) {
    if (str.empty()) return str;

    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\r\n,");
    return str.substr(start, end - start + 1);
}

inline bool load_config(const std::string& filename, Config& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Set defaults first
    config.BIND_ADDRESS = "127.0.0.1";
    config.SERIAL_LISTEN_PORT = 16175;
    config.HTTP_LISTEN_PORT = 16180;
    config.HTTP_AUTH_CREDENTIALS = "passwords";

    // TTGO defaults
    config.TTGO_DEVICE = "/dev/ttyACM0";
    config.TTGO_BAUDRATE = 115200;
    config.TTGO_POWER = 2;
    config.DEFAULT_FREQUENCY = 916000000; // 916.0 MHz

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }

        std::string key = trim_config_value(line.substr(0, equals));
        std::string value = trim_config_value(line.substr(equals + 1));

        if (key == "BIND_ADDRESS") {
            config.BIND_ADDRESS = value;
        } else if (key == "SERIAL_LISTEN_PORT") {
            config.SERIAL_LISTEN_PORT = std::stoul(value);
        } else if (key == "HTTP_LISTEN_PORT") {
            config.HTTP_LISTEN_PORT = std::stoul(value);
        } else if (key == "HTTP_AUTH_CREDENTIALS") {
            config.HTTP_AUTH_CREDENTIALS = value;
        } else if (key == "TTGO_DEVICE") {
            config.TTGO_DEVICE = value;
        } else if (key == "TTGO_BAUDRATE") {
            config.TTGO_BAUDRATE = std::stoul(value);
        } else if (key == "TTGO_POWER") {
            config.TTGO_POWER = std::stoi(value);
        } else if (key == "DEFAULT_FREQUENCY") {
            config.DEFAULT_FREQUENCY = std::stoull(value);
        }
    }

    return true;
}
