#pragma once
namespace srotalekh
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