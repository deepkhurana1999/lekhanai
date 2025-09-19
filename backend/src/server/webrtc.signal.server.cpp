#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
#include "server/webrtc.server.hpp"
#include "server/webrtc.signal.server.hpp"

using json = nlohmann::json;
namespace hikki
{

    WebRTCSignalingServer::WebRTCSignalingServer()
    {
        // Set up WebSocket server
        wsServer.set_access_channels(websocketpp::log::alevel::all);
        wsServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
        wsServer.init_asio();

        // Set WebSocket handlers
        wsServer.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg)
                                     { handleWebSocketMessage(hdl, msg); });

        wsServer.set_open_handler([this](websocketpp::connection_hdl hdl)
                                  {
            std::cout << "WebSocket connection opened" << std::endl;
            // Create a new WebRTC server instance for this client
            clients[hdl] = std::make_shared<WebRTCServer>(); });

        wsServer.set_close_handler([this](websocketpp::connection_hdl hdl)
                                   {
            std::cout << "WebSocket connection closed" << std::endl;
            clients.erase(hdl); });
    }

    void WebRTCSignalingServer::run(int port)
    {
        wsServer.listen(port);
        wsServer.start_accept();

        std::cout << "WebRTC Signaling Server running on port " << port << std::endl;
        wsServer.run();
    }

    void WebRTCSignalingServer::handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
    {
        try
        {
            json message = json::parse(msg->get_payload());
            auto client_it = clients.find(hdl);

            if (client_it == clients.end())
            {
                std::cerr << "No WebRTC client found for connection" << std::endl;
                return;
            }

            auto webrtc_client = client_it->second;

            if (message["type"] == "offer")
            {
                webrtc_client->handleOfferFromClient(message["sdp"]);
            }
            else if (message["type"] == "ice-candidate")
            {
                webrtc_client->handleCandidateFromClient(
                    message["candidate"],
                    message["sdpMid"]);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
        }
    }

    void WebRTCSignalingServer::sendSDPToClient(const std::string &sdp) override
    {
        // Find the corresponding WebSocket connection and send SDP
        json response = {
            {"type", "answer"},
            {"sdp", sdp}};

        // Send to the appropriate client (you'll need to track which connection this is for)
        // This is a simplified example - in practice you'd need better connection management
        for (auto &[hdl, client] : clients)
        {
            wsServer.send(hdl, response.dump(), websocketpp::frame::opcode::text);
            break; // Send to first client for this example
        }
    }

    void WebRTCSignalingServer::sendCandidateToClient(const std::string &candidate, const std::string &mid) override
    {
        json response = {
            {"type", "ice-candidate"},
            {"candidate", candidate},
            {"sdpMid", mid}};

        for (auto &[hdl, client] : clients)
        {
            wsServer.send(hdl, response.dump(), websocketpp::frame::opcode::text);
            break;
        }
    }
}