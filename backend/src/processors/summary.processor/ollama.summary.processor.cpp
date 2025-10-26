#include <string>
#include <iostream>
#include "ollama.hpp"
#include "processors/summary.processor/summary.processor.hpp"
#include "processors/summary.processor/ollama.summary.processor.hpp"

namespace lekhanai
{
    OllamaSummaryProcessor::OllamaSummaryProcessor(std::string &model, std::string &server_url) : SummaryProcessor(model), server_url(server_url)
    {
        ollama_server = new Ollama(server_url);
    }

    OllamaSummaryProcessor::~OllamaSummaryProcessor()
    {
        delete ollama_server;
    }

    std::string OllamaSummaryProcessor::process(const std::string &transcription)
    {
        try
        {
            std::string command = "Analyse the text, identify the domain, get appropriate information regarding the domain, analyse the context, generate the summary using analyzed text, identified domain, identified domain knowledge, identified context for text: " + transcription;
            return ollama_server->generate(model, command).as_simple_string();
        }
        catch (const std::exception &e)
        {
            // Log the error message if needed
            std::cerr << "Error occurred while processing transcription: " << e.what() << std::endl;
            return "[SUMMARY_ERROR]";
        }
    }
}