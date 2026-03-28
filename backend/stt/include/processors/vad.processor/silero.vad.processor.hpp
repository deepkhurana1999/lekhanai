#pragma once
#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include "vad.processor.hpp"

namespace lekhanai
{
    class SileroVADProcessor : public VADProcessor
    {
    public:
        SileroVADProcessor(const std::string &model_path,
                           int windows_frame_size,
                           int sample_rate,
                           int effective_window_size);
        float process(std::vector<float> &chunk, std::vector<float> &state) override;
        void reset_states() override;

    private:
        // ONNX Runtime resources
        Ort::Env env;
        Ort::SessionOptions session_options;
        std::shared_ptr<Ort::Session> session = nullptr;
        Ort::AllocatorWithDefaultOptions allocator;
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPU);

        // ONNX Runtime input/output buffers
        std::vector<Ort::Value> ort_inputs;
        std::vector<const char *> input_node_names = {"input", "state", "sr"};
        std::vector<int64_t> sr;
        int64_t input_node_dims[2] = {};
        const int64_t state_node_dims[3] = {2, 1, 128};
        const int64_t sr_node_dims[1] = {1};
        std::vector<Ort::Value> ort_outputs;
        std::vector<const char *> output_node_names = {"output", "stateN"};

        void init_onnx_model(const std::string &model_path);
        void init_engine_threads(int inter_threads, int intra_threads);
    };
}