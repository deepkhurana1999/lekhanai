// Boost.Asio HTTP server for VAD and Whisper endpoints
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <string>
#include <thread>
#include "processors/request.processor.hpp"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

namespace lekhanai
{

    class HttpServer
    {
    public:
        HttpServer(boost::asio::io_context &ioc, unsigned short port, RequestProcessor *processor);
        void run(boost::asio::io_context &ioc);

    private:
        tcp::acceptor request_acceptor;
        RequestProcessor *request_processor;

        void processRequest();
        void handleRequest(tcp::socket socket);
        void handlePostRequest(http::request<http::string_body> &req, http::response<http::string_body> &res);
        void handleProcessRequest(http::request<http::string_body> &req, http::response<http::string_body> &res);
    };
}