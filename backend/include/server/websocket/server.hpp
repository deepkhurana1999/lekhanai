#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "processors/request.processor.hpp"

namespace lekhanai
{
    class WebSocketServer
    {
    private:
        typedef websocketpp::server<websocketpp::config::asio> server;
        server ws_server;
        RequestProcessor *request_processor;

    public:
        WebSocketServer();

        void run(int port);
        void setRequestProcessor(RequestProcessor *processor);

    private:
        void handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
    };
}