#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "processors/index.hpp"

namespace srotalekh
{
    class WebSocketServer
    {
    private:
        typedef websocketpp::server<websocketpp::config::asio> server;
        server wsServer;
        Processor *requestProcessor;

    public:
        WebSocketServer();

        void run(int port);
        void setRequestProcessor(Processor *processor);

    private:
        void handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
    };
}