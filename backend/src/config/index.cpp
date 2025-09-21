#include <cstdlib>
#include <string>
#include <stdexcept>
#include "config/index.hpp"

namespace hikki
{
    Config Environment::getConfig()
    {
        Config config;
        config.serverPort = get("SERVER_PORT");
        config.modelPath = get("MODEL_PATH");
        return config;
    }

    std::string Environment::get(const std::string &key)
    {
        const char *value = std::getenv(key.c_str());
        if (!value)
        {
            throw std::runtime_error("Environment variable not found");
        }
        return std::string(value);
    }
}
