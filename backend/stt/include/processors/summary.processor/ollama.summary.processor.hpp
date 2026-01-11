#pragma once
#include <string>
#include "ollama.hpp"
#include "summary.processor.hpp"

namespace lekhanai
{
    class OllamaSummaryProcessor : public SummaryProcessor
    {
    public:
        explicit OllamaSummaryProcessor(std::string &model, std::string &server_url);
        std::string process(const std::string &text) override;
        ~OllamaSummaryProcessor();

    private:
        std::string server_url;
        Ollama *ollama_server;
    };
}