#pragma once
#include <string>
#include "constants.hpp"
#include "processors/summary.processor/summary.processor.hpp"

namespace lekhanai
{
    class SummaryProcessorFactory
    {
    public:
        SummaryProcessorFactory();
        SummaryProcessor *create(std::string &model, std::string &server_url, std::string &provider_type);
    };
}