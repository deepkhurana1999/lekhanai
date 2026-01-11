#pragma once
namespace lekhanai
{
    enum class ServerType
    {
        WebSocket,
        WebRTCSignaling,
        Http
    };

    class Server
    {
    public:
        void run(int port, ServerType type);
    };
}