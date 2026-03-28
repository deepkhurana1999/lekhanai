#include "server/server.hpp"
#include "server/http/server.hpp"
#include "server/websocket/server.hpp"
#include "server/webrtc/signal.server.hpp"
#include "processors/request.processor.hpp"

namespace lekhanai
{

    void Server::run(int port, ServerType type)
    {
        if (type == ServerType::WebSocket)
        {
            WebSocketServer wsServer;
            RequestProcessor *processor = new RequestProcessor();
            wsServer.setRequestProcessor(processor);
            wsServer.run(port);
        }
        else if (type == ServerType::WebRTCSignaling)
        {
            WebRTCSignalingServer webrtcServer;
            webrtcServer.run(port);
        }
        else if (type == ServerType::Http)
        {
            boost::asio::io_context ioc;
            RequestProcessor *processor = new RequestProcessor();
            HttpServer http_server(ioc, port, processor);
            std::cout << "HTTP Server running on port " << port << std::endl;
            http_server.run(ioc);
        }
        else
        {
            throw std::invalid_argument("Invalid server type");
        }
    }
}