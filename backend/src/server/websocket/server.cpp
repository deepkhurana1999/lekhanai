#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
#include "server/websocket/server.hpp"
#include "processors/index.hpp"

using json = nlohmann::json;
namespace lekhanai
{
    WebSocketServer::WebSocketServer()
    {
        // Set up WebSocket server
        wsServer.set_access_channels(websocketpp::log::alevel::all);
        wsServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
        wsServer.init_asio();

        // Set WebSocket handlers
        wsServer.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg)
                                     { handleWebSocketMessage(hdl, msg); });

        wsServer.set_open_handler([this](websocketpp::connection_hdl hdl)
                                  { std::cout << "WebSocket connection opened" << std::endl; });

        wsServer.set_close_handler([this](websocketpp::connection_hdl hdl)
                                   { std::cout << "WebSocket connection closed" << std::endl; });
    }

    void WebSocketServer::run(int port)
    {
        wsServer.listen(port);
        wsServer.start_accept();

        std::cout << "WebSocket Server running on port " << port << std::endl;
        wsServer.run();
    }

    void WebSocketServer::setRequestProcessor(Processor *processor)
    {
        requestProcessor = processor;
    }

    void WebSocketServer::handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
    {
        try
        {
            if (msg->get_opcode() == websocketpp::frame::opcode::binary)
            {
                std::vector<float> audio_data = requestProcessor->getDecodedAudio(msg->get_payload());
                auto speech_segments = requestProcessor->getSpeechSegments(audio_data);
                if (speech_segments.empty())
                {
                    json response{
                        {"type", "transcription"},
                        {"text", ""},
                        {"vad_score", 0.0}};
                    wsServer.send(hdl, response.dump(), websocketpp::frame::opcode::text);
                    return;
                }

                for (const auto &segment : speech_segments)
                {
                    std::vector<float> segmented_audio(audio_data.begin() + segment.start, audio_data.begin() + segment.end + 1);
                    std::string transcription = requestProcessor->getVoiceTranscription(segmented_audio);
                    json response{
                        {"type", "transcription"},
                        {"text", transcription},
                        {"vad_score", 1.0}};
                    wsServer.send(hdl, response.dump(), websocketpp::frame::opcode::text);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
        }
    }
}