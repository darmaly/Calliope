#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <map>
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
    double totalBeats = 0.0;      // length in beats (0 = up to the end of the last clip)
};

struct StemExportOptions {
    std::string outputDir;        // directory for stems
    double totalBeats = 0.0;      // 0 = up to the end of the last clip
    std::map<std::string, std::string> trackNames;  // track ID -> file name (without extension)
};

using ProgressCallback = std::function<void(float)>; // 0.0 to 1.0

// Renders the arrangement the engine currently holds (its tracks, clips,
// effects and mix) to audio files. What is exported is what plays.
class AudioExporter {
public:
    explicit AudioExporter(Engine& engine);

    // Export full mix to file. Returns true on success.
    bool exportMix(const ExportOptions& options, ProgressCallback progress = nullptr);

    // Export one 24-bit WAV per track. Returns true on success.
    bool exportStems(const StemExportOptions& options, ProgressCallback progress = nullptr);

    // MP3 needs the LAME encoder, which is an optional build dependency.
    static bool isFormatSupported(ExportFormat format);

    // Parse MIDI events JSON array using exact camelCase field names:
    // beatPosition, noteNumber, velocity, durationBeats, trackId
    static std::vector<MidiEventData> parseMidiEventsJson(const juce::String& jsonString);

private:
    Engine& engine_;

    double resolveTotalBeats(double requested) const;

    bool writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                  double sampleRate, int bitDepth);
    bool writeFlac(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                   double sampleRate);
    bool writeMp3(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                  double sampleRate, int bitrate);
};

} // namespace calliope
