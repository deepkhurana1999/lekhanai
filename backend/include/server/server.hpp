#pragma once
namespace hikki
{
    enum class ServerType
    {
        WebSocket,
        WebRTCSignaling
    };

    class Server
    {
    public:
        void run(int port, ServerType type);
    };
}