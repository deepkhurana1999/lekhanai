#include <cstdlib>
#include <string>
#include <stdexcept>
#include "config/index.hpp"

namespace hikki
{
    Config Environment::config{};
    bool Environment::initialized = false;

    Config Environment::getConfig()
    {
        if (initialized)
        {
            return config;
        }
        config = Config();
        initialized = true;
        config.serverPort = std::stoi(get("SERVER_PORT"));
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
