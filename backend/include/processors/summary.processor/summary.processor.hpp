#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace lekhanai
{
    class SummaryProcessor
    {
    public:
        explicit SummaryProcessor(std::string &model);
        virtual std::string process(const std::string &text) = 0;

    protected:
        std::mutex threadMutex; // Guard for thread safety if needed
        std::string model;
    };
}