// =============================================================================
// DEPRECATED: WebSocket server has been replaced by the Manager service WebSocket
// bridge (/ws/transcribe/{session_id}). STT now operates as a pure HTTP service.
// This file is kept for reference only and is NOT used in the active code path.
// =============================================================================
#include <vector>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include "constants.hpp"
#include "server/websocket/server.hpp"
#include "processors/request.processor.hpp"

using json = nlohmann::json;
namespace lekhanai
{
    WebSocketServer::WebSocketServer()
    {
        // Set up WebSocket server
        ws_server.set_access_channels(websocketpp::log::alevel::all);
        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
        ws_server.init_asio();

        // Set WebSocket handlers
        ws_server.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg)
                                      { handleWebSocketMessage(hdl, msg); });

        ws_server.set_open_handler([this](websocketpp::connection_hdl hdl)
                                   { std::cout << "WebSocket connection opened" << std::endl; });

        ws_server.set_close_handler([this](websocketpp::connection_hdl hdl)
                                    { std::cout << "WebSocket connection closed" << std::endl; });
    }

    void WebSocketServer::run(int port)
    {
        ws_server.listen(port);
        ws_server.start_accept();

        std::cout << "WebSocket Server running on port " << port << std::endl;
        ws_server.run();
    }

    void WebSocketServer::setRequestProcessor(RequestProcessor *processor)
    {
        request_processor = processor;
    }

    void WebSocketServer::handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
    {
        try
        {
            if (msg->get_opcode() == websocketpp::frame::opcode::binary)
            {
                json response = request_processor->process(REQUEST_MESSAGE_TYPE::BINARY, msg->get_payload());
                ws_server.send(hdl, response.dump(), websocketpp::frame::opcode::text);
            }
            else if (msg->get_opcode() == websocketpp::frame::opcode::text)
            {
                json response = request_processor->process(REQUEST_MESSAGE_TYPE::TEXT, msg->get_payload());
                ws_server.send(hdl, response.dump(), websocketpp::frame::opcode::text);
            }
            else
            {
                json response{
                    {"type", "unknown"},
                    {"text", "Unsupported message type"}};
                ws_server.send(hdl, response.dump(), websocketpp::frame::opcode::text);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
            json response{
                {"type", "unknown"},
                {"text", "Failed to process the message"}};
            ws_server.send(hdl, response.dump(), websocketpp::frame::opcode::text);
        }
    }
}