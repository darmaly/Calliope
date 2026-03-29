#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <string>
#include <vector>

namespace calliope {

class Engine;

enum class ExportFormat { WAV_16, WAV_24, MP3, FLAC };

struct MidiEventData {
    double beatPosition;    // matches JS "beatPosition"
    int noteNumber;         // matches JS "noteNumber"
    int velocity;           // matches JS "velocity"
    double durationBeats;   // matches JS "durationBeats"
    std::string trackId;    // matches JS "trackId"
};

struct ExportOptions {
    std::string outputPath;
    ExportFormat format = ExportFormat::WAV_16;
    int mp3Bitrate = 192;         // kbps (128, 192, 256, 320)
    double totalBeats = 0.0;      // length in beats (0 = auto-detect from project)
    double sampleRate = 0.0;      // 0 = use project rate
    std::vector<MidiEventData> midiEvents;
};

struct StemExportOptions {
    std::string outputDir;        // directory for stems
    double totalBeats = 0.0;
    double sampleRate = 0.0;
    std::vector<MidiEventData> midiEvents;
};

using ProgressCallback = std::function<void(float)>; // 0.0 to 1.0

class AudioExporter {
public:
    explicit AudioExporter(Engine& engine);

    // Export full mix to file. Returns true on success.
    bool exportMix(const ExportOptions& options, ProgressCallback progress = nullptr);

    // Export individual track stems as WAV. Returns true on success.
    bool exportStems(const StemExportOptions& options, ProgressCallback progress = nullptr);

    // Parse MIDI events JSON array using exact camelCase field names:
    // beatPosition, noteNumber, velocity, durationBeats, trackId
    static std::vector<MidiEventData> parseMidiEventsJson(const juce::String& jsonString);

private:
    Engine& engine_;

    // Render the graph offline into an AudioBuffer
    juce::AudioBuffer<float> offlineBounce(double totalBeats, double sampleRate,
                                            int bufferSize,
                                            const std::vector<MidiEventData>& midiEvents,
                                            ProgressCallback progress);

    // Render a single track by soloing it (muting all others)
    juce::AudioBuffer<float> offlineBounceSingleTrack(const juce::String& trackId,
                                                       double totalBeats, double sampleRate,
                                                       int bufferSize,
                                                       const std::vector<MidiEventData>& midiEvents);

    bool writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                  double sampleRate, int bitDepth);
    bool writeFlac(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                   double sampleRate);
    bool writeMp3(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                  double sampleRate, int bitrate);
};

} // namespace calliope
