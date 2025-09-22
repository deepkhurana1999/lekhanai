#pragma once
#include <string>

// generate static class to read environment variables
namespace srotalekh
{
    struct Config
    {
        int serverPort;
        std::string modelPath;
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
