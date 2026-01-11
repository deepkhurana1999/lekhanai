// Boost.Asio HTTP server for VAD and Whisper endpoints
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <thread>
#include "server/http/server.hpp"
#include "processors/request.processor.hpp"
#include "processors/audio.processor/vad.audio.processor.hpp"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

namespace lekhanai
{

    HttpServer::HttpServer(boost::asio::io_context &ioc, unsigned short port, RequestProcessor *processor)
        : request_acceptor(ioc, tcp::endpoint(tcp::v4(), port)), processor_(processor)
    {
        do_accept();
    }

    void HttpServer::processRequest()
    {
        request_acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    std::thread(&HttpServer::handle_session, this, std::move(socket)).detach();
                }
                handleRequest();
            });
    }

    void HttpServer::handleRequest(tcp::socket socket)
    {
        try
        {
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req);

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "stt-http");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());

            switch (req.method())
            {
            case http::verb::post:
                handlePostRequest(req, res);
                break;
            default:
                res.result(http::status::not_found);
                res.body() = "{\"error\":\"Not found\"}";
                break;
            }

            res.prepare_payload();
            http::write(socket, res);
        }
        catch (std::exception &e)
        {
            std::cerr << "HTTP session error: " << e.what() << std::endl;
        }
    }

    void HttpServer::handlePostRequest(http::request<http::string_body> &req, http::response<http::string_body> &res)
    {
        switch (req.target())
        {
        case "/api/v1/vad/detect":
        {
            // Expect JSON: { "audio": [float, float, ...] }
            auto input_json = nlohmann::json::parse(req.body());
            std::vector<float> audio = input_json["audio"].get<std::vector<float>>();
            auto segments = request_processor->getSpeechSegments(audio);
            nlohmann::json out;
            out["segments"] = nlohmann::json::array();
            for (const auto &seg : segments)
            {
                out["segments"].push_back({{"start", seg.start}, {"end", seg.end}});
            }
            res.body() = out.dump();
            break;
        }
        case "/api/v1/whisper/transcribe":
        {
            // Expect JSON: { "segments": [ {"start":int, "end":int}, ... ], "audio": [float, float, ...] }
            auto input_json = nlohmann::json::parse(req.body());
            std::vector<std::string> audio = input_json["audio"].get<std::vector<std::string>>();
            std::vector<SpeechSegment> segments;
            for (const auto &seg : input_json["segments"])
            {
                int start = seg["start"], end = seg["end"];
                segments.push_back(SpeechSegment(start, end));
            }
            std::string text = request_processor->getVoiceTranscriptions(segments, audio);
            nlohmann::json out;
            out["text"] = text;
            res.body() = out.dump();
            break;
        }
        default:
        {
            res.result(http::status::not_found);
            res.body() = "{\"error\":\"Not found\"}";
            break;
        }
        }
    }
}
