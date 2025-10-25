#include <cstdlib>
#include <string>
#include <stdexcept>
#include "config/index.hpp"

namespace lekhanai
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
        config.server_port = std::stoi(get("SERVER_PORT"));
        config.model_path = get("MODEL_PATH");
        config.vad_model_path = get("VAD_MODEL_PATH");
        config.llm_model = get("LLM_MODEL");
        config.llm_server_url = get("LLM_SERVER_URL");
        config.llm_model_provider = get("LLM_MODEL_PROVIDER");
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
