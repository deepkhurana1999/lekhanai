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
        : request_acceptor(ioc, tcp::endpoint(tcp::v4(), port)), request_processor(processor)
    {
    }

    void HttpServer::run(boost::asio::io_context &ioc)
    {
        processRequest();
        ioc.run();
    }

    void HttpServer::processRequest()
    {
        request_acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    std::thread([this](tcp::socket s) mutable
                                { handleRequest(std::move(s)); }, std::move(socket))
                        .detach();
                }
                processRequest();
            });
    }

    void HttpServer::handleRequest(tcp::socket socket)
    {
        try
        {
            boost::beast::flat_buffer buffer;
            http::request_parser<http::string_body> parser;
            parser.body_limit(50ULL * 1024 * 1024);
            http::read(socket, buffer, parser);
            auto req = parser.release();

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "stt-http");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            if (req.method() == http::verb::post)
            {
                handlePostRequest(req, res);
            }
            else
            {
                res.result(http::status::not_found);
                res.body() = "{\"error\":\"Not found\"}";
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
        auto target = std::string(req.target());
        if (target == "/api/v1/vad")
        {
            // Expect JSON: { "audio": [float, float, ...] }
            auto input_json = nlohmann::json::parse(req.body());
            VADState vad_state;
            // Extract audio and VAD state if provided
            std::vector<float> audio = input_json["audio"].get<std::vector<float>>();
            vad_state.state = input_json.value("state", std::vector<float>{});
            vad_state.context = input_json.value("context", std::vector<float>{});
            vad_state.current_speech.start = input_json.value("current_speech_start", -1);
            vad_state.current_speech.end = input_json.value("current_speech_end", -1);
            vad_state.triggered = input_json.value("triggered", false);

            // Process VAD
            auto segments = request_processor->getSpeechSegments(audio, vad_state);

            // Prepare response
            nlohmann::json out;
            out["current_speech_start"] = vad_state.current_speech.start;
            out["current_speech_end"] = vad_state.current_speech.end;
            out["triggered"] = vad_state.triggered;
            out["segments"] = nlohmann::json::array();
            out["state"] = nlohmann::json::array();
            for (const auto &seg : segments)
            {
                out["segments"].push_back({{"start", seg.start}, {"end", seg.end}});
            }
            for (const auto &s : vad_state.state)
            {
                out["state"].push_back(s);
            }
            for (const auto &c : vad_state.context)
            {
                out["context"].push_back(c);
            }

            res.body() = out.dump();
        }
        else if (target == "/api/v1/transcribe")
        {
            // Expect JSON: { "segments": [ {"start":int, "end":int}, ... ], "audio": [float, float, ...] }
            auto input_json = nlohmann::json::parse(req.body());
            std::vector<float> audio = input_json["audio"].get<std::vector<float>>();
            std::vector<SpeechSegment> segments;
            for (const auto &seg : input_json["segments"])
            {
                int start = seg["start"], end = seg["end"];
                segments.push_back(SpeechSegment(start, end));
            }
            std::vector<std::string> transcriptions = request_processor->getVoiceTranscriptions(segments, audio);
            std::string text;
            for (const auto &t : transcriptions)
                text += t + " ";
            nlohmann::json out;
            out["text"] = text;
            res.body() = out.dump();
        }
        else if (target == "/api/v1/process")
        {
            // Combined endpoint: accepts raw binary PCM int16 (16kHz, mono)
            // Content-Type: application/octet-stream
            // Runs VAD + Whisper in one call — no double transfer, no JSON float inflation
            handleProcessRequest(req, res);
        }
        else
        {
            res.result(http::status::not_found);
            res.body() = "{\"error\":\"Not found\"}";
        }
    }

    void HttpServer::handleProcessRequest(http::request<http::string_body> &req, http::response<http::string_body> &res)
    {
        try
        {
            const std::string &body = req.body();

            if (body.empty())
            {
                res.result(http::status::bad_request);
                res.body() = "{\"error\":\"Empty audio body\"}";
                return;
            }

            if (body.size() % 2 != 0)
            {
                res.result(http::status::bad_request);
                res.body() = "{\"error\":\"Invalid PCM data: odd byte count\"}";
                return;
            }

            // Convert int16 PCM -> float32 [-1, 1]
            const size_t sample_count = body.size() / 2;
            const int16_t *pcm = reinterpret_cast<const int16_t *>(body.data());
            std::vector<float> audio(sample_count);
            constexpr float norm = 1.0f / 32768.0f;
            for (size_t i = 0; i < sample_count; ++i)
                audio[i] = static_cast<float>(pcm[i]) * norm;

            // VAD -> speech segments
            VADState vad_state;
            auto segments = request_processor->getSpeechSegments(audio, vad_state);

            // Whisper transcription
            std::vector<std::string> transcriptions = request_processor->getVoiceTranscriptions(segments, audio);
            std::string text;
            for (const auto &t : transcriptions)
                text += t + " ";

            nlohmann::json out;
            out["text"] = text;
            res.body() = out.dump();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in /api/v1/process: " << e.what() << std::endl;
            res.result(http::status::internal_server_error);
            nlohmann::json err;
            err["error"] = e.what();
            res.body() = err.dump();
        }
    }
}
