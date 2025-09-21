// src/test_setup.cpp
#include <rtc/rtc.hpp>
#include <whisper.h>
#include <iostream>
#include <thread>
#include "server/server.hpp"
#include "config/index.hpp"

namespace hikki
{
    int main()
    {
        try
        {
            Server server;
            Config config = Environment::getConfig();
            server.run(config.serverPort, ServerType::WebSocket);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Server error: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

}

int main()
{
    return hikki::main();
}