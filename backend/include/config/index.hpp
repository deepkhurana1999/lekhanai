#pragma once
#include <string>

// generate static class to read environment variables
namespace hikki
{
    struct Config
    {
        std::string serverPort;
        std::string modelPath;
    };

    class Environment
    {
    public:
        static Config getConfig();

    private:
        static std::string get(const std::string &key);
    };
}
