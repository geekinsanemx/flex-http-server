#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>

struct Config {
    std::string BIND_ADDRESS;
    uint32_t SERIAL_LISTEN_PORT;
    uint32_t HTTP_LISTEN_PORT;
    uint64_t SAMPLE_RATE;
    uint32_t BITRATE;
    int8_t AMPLITUDE;
    uint32_t FREQ_DEV;
    uint8_t TX_GAIN;
    uint64_t DEFAULT_FREQUENCY;
    std::string HTTP_AUTH_CREDENTIALS; // New field for password file path
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
    config.SAMPLE_RATE = 2000000;
    config.BITRATE = 1600;
    config.AMPLITUDE = 127;
    config.FREQ_DEV = 2400;
    config.TX_GAIN = 0;
    config.DEFAULT_FREQUENCY = 931937500;
    config.HTTP_AUTH_CREDENTIALS = "passwords";

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
        } else if (key == "SAMPLE_RATE") {
            config.SAMPLE_RATE = std::stoull(value);
        } else if (key == "BITRATE") {
            config.BITRATE = std::stoul(value);
        } else if (key == "AMPLITUDE") {
            config.AMPLITUDE = static_cast<int8_t>(std::stoi(value));
        } else if (key == "FREQ_DEV") {
            config.FREQ_DEV = std::stoul(value);
        } else if (key == "TX_GAIN") {
            config.TX_GAIN = static_cast<uint8_t>(std::stoi(value));
        } else if (key == "DEFAULT_FREQUENCY") {
            config.DEFAULT_FREQUENCY = std::stoull(value);
        } else if (key == "HTTP_AUTH_CREDENTIALS") {
            config.HTTP_AUTH_CREDENTIALS = value;
        }
    }

    return true;
}
