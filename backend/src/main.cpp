// src/test_setup.cpp
#include <rtc/rtc.hpp>
#include <whisper.h>
#include <iostream>
#include <thread>
#include "server/webrtc.signal.server.hpp"

namespace hikki
{
    int main1()
    {
        std::cout << "Testing library integration..." << std::endl;

        // Test libdatachannel
        try
        {
            rtc::Configuration config;
            std::cout << "✓ libdatachannel: OK" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "✗ libdatachannel: " << e.what() << std::endl;
        }

        // Test whisper.cpp
        const char *whisper_version = whisper_print_system_info();
        if (whisper_version)
        {
            std::cout << "✓ whisper.cpp: OK" << std::endl;
            std::cout << "  System info: " << whisper_version << std::endl;
        }
        else
        {
            std::cout << "✗ whisper.cpp: Failed to get system info" << std::endl;
        }

        std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;

        return 0;
    }

    int main()
    {
        try
        {
            WebRTCSignalingServer server;
            server.run(8080);
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