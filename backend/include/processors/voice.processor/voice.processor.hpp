#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace lekhanai
{
    class VoiceProcessor
    {
    public:
        explicit VoiceProcessor(const std::string &model_path);
        virtual std::string process(const std::vector<float> &pcmData) = 0;

    protected:
        std::mutex threadMutex; // Guard for thread safety if needed
        std::string model_path;
    };
}