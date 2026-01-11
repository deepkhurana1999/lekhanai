#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "server.hpp"

namespace lekhanai
{
    class WebRTCSignalingServer : public WebRTCServer
    {
    private:
        typedef websocketpp::server<websocketpp::config::asio> server;
        server wsServer;
        std::map<websocketpp::connection_hdl, std::shared_ptr<WebRTCServer>,
                 std::owner_less<websocketpp::connection_hdl>>
            clients;

    public:
        WebRTCSignalingServer();
        void run(int port);

    private:
        void handleWebSocketMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);

    protected:
        void sendSDPToClient(const std::string &sdp) override;

        void sendCandidateToClient(const std::string &candidate, const std::string &mid) override;
    };
}