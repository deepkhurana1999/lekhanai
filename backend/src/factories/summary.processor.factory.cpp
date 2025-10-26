#include <stdexcept>
#include <string>
#include "constants.hpp"
#include "factories/summary.processor.factory.hpp"
#include "processors/summary.processor/ollama.summary.processor.hpp"

namespace lekhanai
{
    SummaryProcessorFactory::SummaryProcessorFactory() = default;
    SummaryProcessor *SummaryProcessorFactory::create(std::string &model, std::string &server_url, std::string &provider_type)
    {
        if (provider_type == "ollama")
        {
            return new OllamaSummaryProcessor(model, server_url);
        }

        throw std::invalid_argument("Unsupported LLM provider type: " + provider_type);
    }
}