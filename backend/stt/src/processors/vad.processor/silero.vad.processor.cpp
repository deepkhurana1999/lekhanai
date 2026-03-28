#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include "processors/vad.processor/vad.processor.hpp"
#include "processors/vad.processor/silero.vad.processor.hpp"

namespace lekhanai
{

    SileroVADProcessor::SileroVADProcessor(
        const std::string &model_path,
        int windows_frame_size,
        int sample_rate,
        int effective_window_size)
        : VADProcessor(model_path, windows_frame_size, sample_rate, effective_window_size) //, env(ORT_LOGGING_LEVEL_WARNING, "SileroVAD")
    {
        input_node_dims[0] = 1;
        input_node_dims[1] = effective_window_size;
        sr.resize(1);
        sr[0] = sample_rate;
        init_onnx_model(model_path);
    }

    void SileroVADProcessor::init_onnx_model(const std::string &model_path)
    {
        init_engine_threads(1, 1);
        session = std::make_shared<Ort::Session>(env, model_path.c_str(), session_options);
    }

    void SileroVADProcessor::init_engine_threads(int inter_threads, int intra_threads)
    {
        session_options.SetIntraOpNumThreads(intra_threads);
        session_options.SetInterOpNumThreads(inter_threads);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    }

    void SileroVADProcessor::reset_states()
    {
        // No internal states to reset in this implementation.
    }

    float SileroVADProcessor::process(std::vector<float> &data_chunk, std::vector<float> &state)
    {
        std::vector<float> input = data_chunk;

        // Create input tensor (input_node_dims[1] is already set to effective_window_size).
        Ort::Value input_ort = Ort::Value::CreateTensor<float>(
            memory_info, input.data(), input.size(), input_node_dims, 2);
        Ort::Value state_ort = Ort::Value::CreateTensor<float>(
            memory_info, state.data(), state.size(), state_node_dims, 3);
        Ort::Value sr_ort = Ort::Value::CreateTensor<int64_t>(
            memory_info, sr.data(), sr.size(), sr_node_dims, 1);
        ort_inputs.clear();
        ort_inputs.emplace_back(std::move(input_ort));
        ort_inputs.emplace_back(std::move(state_ort));
        ort_inputs.emplace_back(std::move(sr_ort));

        // Run inference.
        ort_outputs = session->Run(
            Ort::RunOptions{nullptr},
            input_node_names.data(), ort_inputs.data(), ort_inputs.size(),
            output_node_names.data(), output_node_names.size());

        float speech_prob = ort_outputs[0].GetTensorMutableData<float>()[0];
        float *stateN = ort_outputs[1].GetTensorMutableData<float>();
        std::memcpy(state.data(), stateN, state.size() * sizeof(float));

        return speech_prob;
    }
}