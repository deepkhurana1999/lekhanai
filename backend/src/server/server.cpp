#include "server/server.hpp"
#include "server/websocket/server.hpp"
#include "server/webrtc/signal.server.hpp"

namespace hikki
{

    void Server::run(int port, ServerType type)
    {
        if (type != ServerType::WebRTCSignaling && type != ServerType::WebSocket)
        {
            throw std::invalid_argument("Invalid server type");
        }

        if (type == ServerType::WebSocket)
        {
            WebSocketServer wsServer;
            wsServer.run(port);
        }
        else if (type == ServerType::WebRTCSignaling)
        {
            WebRTCSignalingServer webrtcServer;
            webrtcServer.run(port);
        }
    }
}