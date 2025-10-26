#include <iostream>
#include <nlohmann/json.hpp>
#include "constants.hpp"
#include "config/index.hpp"
#include "processors/index.hpp"
#include "factories/vad.processor.factory.hpp"
#include "factories/voice.processor.factory.hpp"
#include "factories/summary.processor.factory.hpp"
#include "processors/audio.processor/audio.processor.hpp"
#include "processors/audio.processor/vad.audio.processor.hpp"

using json = nlohmann::json;

namespace lekhanai
{
    Processor::Processor()
    {
        Config config = Environment::getConfig();
        const int sample_rate = 16000; // Fixed sample rate for VAD and AudioProcessor
        const int windows_frame_size = 32;
        const int sr_per_ms = sample_rate / 1000;                       // e.g., 16000 / 1000 = 16
        const int window_size_samples = windows_frame_size * sr_per_ms; // e.g., 32ms * 16 = 512 samples
        const int min_silence_duration_ms = 100;
        const int speech_pad_samples = 30;
        const int min_speech_duration_ms = 250;
        const int context_samples = 64; // For 16kHz, 64 samples are added as context.
        const float voice_activation_threshold = 0.5;
        const int effective_window_size = window_size_samples + context_samples;
        const float max_speech_duration_s = 10.0f;
        const int min_speech_samples = sr_per_ms * min_speech_duration_ms;
        const int max_speech_samples = (sample_rate * max_speech_duration_s - window_size_samples - 2 * speech_pad_samples);
        const int min_silence_samples = sr_per_ms * min_silence_duration_ms;
        const int min_silence_samples_at_max_speech = sr_per_ms * 98;

        // Initialize processors
        const int n_threads = 4;
        const int n_processors = 4;

        audio_processor = new AudioProcessor();

        voice_processor = VoiceProcessorFactory().create(
            config.model_path,
            STT_MODEL::WHISPER,
            n_threads,
            n_processors);

        vad_processor = VADProcessorFactory().create(
            config.vad_model_path,
            VAD_MODEL::SILERO,
            windows_frame_size,
            sample_rate,
            effective_window_size);

        vad_audio_processor = new VADAudioProcessor(
            vad_processor,
            sample_rate,
            context_samples,
            window_size_samples,
            effective_window_size,
            min_silence_samples,
            min_silence_samples_at_max_speech,
            min_speech_samples,
            max_speech_samples,
            speech_pad_samples,
            voice_activation_threshold);

        summary_processor = SummaryProcessorFactory().create(
            config.llm_model,
            config.llm_server_url,
            config.llm_model_provider);
    }

    json Processor::process(REQUEST_MESSAGE_TYPE request_message_type, const std::string &payload)
    {
        try
        {
            if (request_message_type == REQUEST_MESSAGE_TYPE::BINARY)
            {
                std::vector<std::string> transcriptions = getVoiceTranscriptions(payload);
                std::string complete_transcription = "";
                for (auto &transcription : transcriptions)
                {
                    complete_transcription += transcription + " ";
                }
                json response{
                    {"type", "transcription"},
                    {"text", complete_transcription}};
                return response;
            }
            else if (request_message_type == REQUEST_MESSAGE_TYPE::TEXT)
            {
                auto received_json = json::parse(payload);
                if (received_json["type"] == "generate_summary")
                {
                    std::string transcription_text = received_json["text"];
                    std::string summary = getSummary(transcription_text);
                    json response{
                        {"type", "summary"},
                        {"text", summary}};
                    return response;
                }
            }
            else
            {
                json response{
                    {"type", "unknown"},
                    {"text", "Unsupported message type"}};
                return response;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling message: " << e.what() << std::endl;
            std::cerr << "Payload: " << payload << std::endl;

            json response{
                {"type", "unknown"},
                {"text", "Failed to process the message"}};

            return response;
        }
    }

    std::vector<float> Processor::getDecodedAudio(const std::string &raw_audio)
    {
        return audio_processor->process(raw_audio);
    }

    std::string Processor::getSummary(const std::string &transcription)
    {
        return summary_processor->process(transcription);
    }

    std::string Processor::getVoiceTranscription(const std::vector<std::vector<float>> &audio)
    {
        return voice_processor->process(audio);
    }

    std::vector<SpeechSegment> Processor::getSpeechSegments(const std::vector<float> &audio)
    {
        return vad_audio_processor->process(audio);
    }

    std::vector<std::string> Processor::getVoiceTranscriptions(const std::string &raw_audio)
    {
        std::vector<std::string> transcriptions;
        std::vector<float> audio_data = getDecodedAudio(raw_audio);
        auto speech_segments = getSpeechSegments(audio_data);
        if (speech_segments.empty())
        {
            transcriptions.push_back("[BLANK_AUDIO]");
            return transcriptions;
        }

        int n_processors = 4;
        int batch_number = -1;
        std::vector<std::vector<std::vector<float>>> batched_audio;
        for (int i = 0; i < speech_segments.size(); ++i)
        {
            // Check if we need a new batch
            if (batch_number == -1 ||
                batched_audio[batch_number].size() >= n_processors)
            {

                // Create new batch
                batched_audio.push_back(std::vector<std::vector<float>>());
                batch_number++;
            }

            std::vector<float> segmented_audio(audio_data.begin() + speech_segments[i].start, audio_data.begin() + speech_segments[i].end + 1);
            batched_audio[batch_number].push_back(segmented_audio);
        }

        for (const auto &batch : batched_audio)
        {
            std::string transcription = getVoiceTranscription(batch);
            transcriptions.push_back(transcription);
        }

        if (transcriptions.empty())
        {
            transcriptions.push_back("[BLANK_AUDIO]");
        }

        return transcriptions;
    }

    Processor::~Processor()
    {
        delete audio_processor;
        delete voice_processor;
        delete vad_processor;
        delete vad_audio_processor;
    }
}