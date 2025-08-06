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

    // FLEX-specific configuration (updated from TTGO)
    std::string FLEX_DEVICE;
    uint32_t FLEX_BAUDRATE;
    int FLEX_POWER;
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

    // FLEX defaults (updated from TTGO)
    config.FLEX_DEVICE = "/dev/ttyUSB0";  // Changed from TTGO_DEVICE
    config.FLEX_BAUDRATE = 115200;        // Changed from TTGO_BAUDRATE
    config.FLEX_POWER = 2;                // Changed from TTGO_POWER, range 2-20
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
        } else if (key == "FLEX_DEVICE") {           // Updated from TTGO_DEVICE
            config.FLEX_DEVICE = value;
        } else if (key == "FLEX_BAUDRATE") {         // Updated from TTGO_BAUDRATE
            config.FLEX_BAUDRATE = std::stoul(value);
        } else if (key == "FLEX_POWER") {            // Updated from TTGO_POWER
            config.FLEX_POWER = std::stoi(value);
        } else if (key == "DEFAULT_FREQUENCY") {
            config.DEFAULT_FREQUENCY = std::stoull(value);
        }

        // Support legacy TTGO_ prefixes for backward compatibility
        else if (key == "TTGO_DEVICE") {
            config.FLEX_DEVICE = value;
        } else if (key == "TTGO_BAUDRATE") {
            config.FLEX_BAUDRATE = std::stoul(value);
        } else if (key == "TTGO_POWER") {
            config.FLEX_POWER = std::stoi(value);
        }
    }

    return true;
}
