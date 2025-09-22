#pragma once
#include <rtc/rtc.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <functional>

namespace srotalekh
{
    class WebRTCServer
    {
    private:
        rtc::Configuration config;
        std::shared_ptr<rtc::PeerConnection> peerConnection;
        std::shared_ptr<rtc::DataChannel> dataChannel;

    public:
        WebRTCServer();

        void setupPeerConnection();

    private:
        void setupDataChannelHandlers(std::shared_ptr<rtc::DataChannel> dc);

        void handleTextMessage(const std::string &message);

        void handleBinaryMessage(const rtc::binary &data);

        // These methods need to be implemented based on your signaling mechanism
        virtual void sendSDPToClient(const std::string &sdp);

        virtual void sendCandidateToClient(const std::string &candidate, const std::string &mid);

    public:
        // Methods to handle signaling from client
        void handleOfferFromClient(const std::string &sdp);

        void handleCandidateFromClient(const std::string &candidate, const std::string &mid);

        // Send data to client
        void sendToClient(const std::string &message);

        void sendBinaryToClient(const rtc::binary &data);
    };
}