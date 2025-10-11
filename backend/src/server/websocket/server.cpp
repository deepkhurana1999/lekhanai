#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include "processors/index.hpp"
#include "server/websocket/server.hpp"

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

    void WebSocketServer::setRequestProcessor(Processor *processor)
    {
        request_processor = processor;
    }

    void WebSocketServer::handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
    {
        try
        {
            if (msg->get_opcode() == websocketpp::frame::opcode::binary)
            {
                std::vector<std::string> transcriptions = request_processor->process(msg->get_payload());
                for (auto &transcription : transcriptions)
                {
                    json response{
                        {"type", "transcription"},
                        {"text", transcription}};
                    ws_server.send(hdl, response.dump(), websocketpp::frame::opcode::text);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
        }
    }
}