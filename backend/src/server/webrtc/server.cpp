#include <rtc/rtc.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include "server/webrtc/server.hpp"

namespace lekhanai
{
    WebRTCServer::WebRTCServer()
    {
        // Configure STUN servers for NAT traversal
        config.iceServers.emplace_back("stun:stun.l.google.com:19302");
        config.iceServers.emplace_back("stun:stun1.l.google.com:19302");

        setupPeerConnection();
    }

    void WebRTCServer::setupPeerConnection()
    {
        peerConnection = std::make_shared<rtc::PeerConnection>(config);

        // Set up connection state handlers
        peerConnection->onStateChange([](rtc::PeerConnection::State state)
                                      { std::cout << "Peer connection state: " << static_cast<int>(state) << std::endl; });

        peerConnection->onIceStateChange([](rtc::PeerConnection::IceState state)
                                         { std::cout << "ICE state: " << static_cast<int>(state) << std::endl; });

        // Handle incoming data channels from client
        peerConnection->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc)
                                      {
            std::cout << "Received data channel: " << dc->label() << std::endl;
            setupDataChannelHandlers(dc); });

        // Handle SDP and ICE candidates
        peerConnection->onLocalDescription([this](rtc::Description desc)
                                           {
            std::cout << "Local description created" << std::endl;
            // Send this SDP to the client via your signaling mechanism
            sendSDPToClient(std::string(desc)); });

        peerConnection->onLocalCandidate([this](rtc::Candidate candidate)
                                         {
            std::cout << "Local candidate: " << candidate.candidate() << std::endl;
            // Send this ICE candidate to the client
            sendCandidateToClient(candidate.candidate(), candidate.mid()); });
    }

    void WebRTCServer::setupDataChannelHandlers(std::shared_ptr<rtc::DataChannel> dc)
    {
        dataChannel = dc;

        // Handle data channel open event
        dc->onOpen([]()
                   { std::cout << "Data channel opened" << std::endl; });

        // Handle incoming messages from client
        dc->onMessage([this](std::variant<rtc::binary, rtc::string> message)
                      {
            if (std::holds_alternative<rtc::string>(message)) {
                std::string text = std::get<rtc::string>(message);
                std::cout << "Received text: " << text << std::endl;
                handleTextMessage(text);
            } else if (std::holds_alternative<rtc::binary>(message)) {
                rtc::binary data = std::get<rtc::binary>(message);
                std::cout << "Received binary data: " << data.size() << " bytes" << std::endl;
                handleBinaryMessage(data);
            } });

        // Handle data channel close
        dc->onClosed([]()
                     { std::cout << "Data channel closed" << std::endl; });

        // Handle errors
        dc->onError([](std::string error)
                    { std::cout << "Data channel error: " << error << std::endl; });
    }

    void WebRTCServer::handleTextMessage(const std::string &message)
    {
        // Process incoming text message
        std::cout << "Processing text message: " << message << std::endl;

        // Echo the message back to client
        if (dataChannel && dataChannel->isOpen())
        {
            dataChannel->send("Echo: " + message);
        }
    }

    void WebRTCServer::handleBinaryMessage(const rtc::binary &data)
    {
        // Process incoming binary data
        std::cout << "Processing binary data of size: " << data.size() << std::endl;

        // Example: Save to file or process the data
        // processFileData(data);

        // Send acknowledgment back to client
        if (dataChannel && dataChannel->isOpen())
        {
            std::string ack = "Received " + std::to_string(data.size()) + " bytes";
            dataChannel->send(ack);
        }
    }

    // These methods need to be implemented based on your signaling mechanism
    void WebRTCServer::sendSDPToClient(const std::string &sdp)
    {
        // Implement your signaling logic here
        std::cout << "Send SDP to client: " << sdp << std::endl;
    }

    void WebRTCServer::sendCandidateToClient(const std::string &candidate, const std::string &mid)
    {
        // Implement your signaling logic here
        std::cout << "Send ICE candidate to client: " << candidate << std::endl;
    }

    // Methods to handle signaling from client
    void WebRTCServer::handleOfferFromClient(const std::string &sdp)
    {
        try
        {
            rtc::Description offer(sdp, rtc::Description::Type::Offer);
            peerConnection->setRemoteDescription(offer);

            // Create answer automatically since we received an offer
            std::cout << "Creating answer for received offer" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling offer: " << e.what() << std::endl;
        }
    }

    void WebRTCServer::handleCandidateFromClient(const std::string &candidate, const std::string &mid)
    {
        try
        {
            rtc::Candidate cand(candidate, mid);
            peerConnection->addRemoteCandidate(cand);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error adding remote candidate: " << e.what() << std::endl;
        }
    }

    // Send data to client
    void WebRTCServer::sendToClient(const std::string &message)
    {
        if (dataChannel && dataChannel->isOpen())
        {
            dataChannel->send(message);
        }
    }

    void WebRTCServer::sendBinaryToClient(const rtc::binary &data)
    {
        if (dataChannel && dataChannel->isOpen())
        {
            dataChannel->send(data);
        }
    }
}