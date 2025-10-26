#pragma once
#include <string>

// generate static class to read environment variables
namespace lekhanai
{
    struct Config
    {
        int server_port;
        std::string model_path;
        std::string vad_model_path;
        std::string llm_model;
        std::string llm_server_url;
        std::string llm_model_provider;
    };

    class Environment
    {
    public:
        static Config getConfig();

    private:
        static std::string get(const std::string &key);
        static Config config;
        static bool initialized;
    };
}
