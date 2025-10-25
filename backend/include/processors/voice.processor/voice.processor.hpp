#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace lekhanai
{
    class VoiceProcessor
    {
    public:
        explicit VoiceProcessor(const std::string &model_path, int n_threads, int n_processors);
        virtual std::string process(const std::vector<std::vector<float>> &batched_audio) = 0;

    protected:
        std::mutex thread_mutex; // Guard for thread safety if needed
        std::string model_path;
        int n_threads;
        int n_processors;
    };
}