#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace hikki
{
    class WebSocketServer
    {
    private:
        typedef websocketpp::server<websocketpp::config::asio> server;
        server wsServer;

    public:
        WebSocketServer();

        void run(int port);

    private:
        void handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
    };
}