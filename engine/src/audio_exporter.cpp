#include "calliope/audio_exporter.h"
#include "calliope/engine.h"
#include "calliope/audio_graph.h"
#include "calliope/transport.h"

#ifdef CALLIOPE_HAS_LAME
#include <lame/lame.h>
#endif

namespace calliope {

namespace {

// Seconds rendered after the last clip so reverb and delay tails are kept.
constexpr double kTailSeconds = 2.0;

juce::String safeFileName(const juce::String& name)
{
    auto cleaned = juce::File::createLegalFileName(name.trim());
    return cleaned.isEmpty() ? juce::String("track") : cleaned;
}

} // namespace

AudioExporter::AudioExporter(Engine& engine) : engine_(engine) {}

bool AudioExporter::isFormatSupported(ExportFormat format)
{
#ifdef CALLIOPE_HAS_LAME
    juce::ignoreUnused(format);
    return true;
#else
    return format != ExportFormat::MP3;
#endif
}

// ---------------------------------------------------------------------------
// MIDI JSON parsing — uses exact camelCase field names from JS contract.
// Kept for callers that still describe notes this way; exports themselves
// render the clips the engine already holds.
// ---------------------------------------------------------------------------

std::vector<MidiEventData> AudioExporter::parseMidiEventsJson(const juce::String& jsonString)
{
    std::vector<MidiEventData> events;

    auto parsed = juce::JSON::parse(jsonString);
    auto* arr = parsed.getArray();
    if (!arr)
        return events;

    for (const auto& item : *arr) {
        if (auto* obj = item.getDynamicObject()) {
            MidiEventData evt;
            evt.beatPosition  = obj->getProperty("beatPosition");
            evt.noteNumber    = static_cast<int>(obj->getProperty("noteNumber"));
            evt.velocity      = static_cast<int>(obj->getProperty("velocity"));
            evt.durationBeats = obj->getProperty("durationBeats");
            evt.trackId       = obj->getProperty("trackId").toString().toStdString();
            events.push_back(evt);
        }
    }

    return events;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

double AudioExporter::resolveTotalBeats(double requested) const
{
    if (requested > 0.0)
        return requested;
    return engine_.getAudioGraph().getClipScheduler().getEndBeat();
}

// ---------------------------------------------------------------------------
// Format writers
// ---------------------------------------------------------------------------

bool AudioExporter::writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                              double sampleRate, int bitDepth)
{
    file.deleteFile();
    auto stream = std::make_unique<juce::FileOutputStream>(file);
    if (!stream->openedOk())
        return false;

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(stream.get(), sampleRate,
                                  static_cast<unsigned int>(buffer.getNumChannels()),
                                  bitDepth, {}, 0));
    if (!writer)
        return false;
    stream.release();  // the writer owns the stream now

    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

bool AudioExporter::writeFlac(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                               double sampleRate)
{
    file.deleteFile();
    auto stream = std::make_unique<juce::FileOutputStream>(file);
    if (!stream->openedOk())
        return false;

    juce::FlacAudioFormat flacFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        flacFormat.createWriterFor(stream.get(), sampleRate,
                                   static_cast<unsigned int>(buffer.getNumChannels()),
                                   24, {}, 0));
    if (!writer)
        return false;
    stream.release();  // the writer owns the stream now

    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

bool AudioExporter::writeMp3(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                              double sampleRate, int bitrate)
{
#ifdef CALLIOPE_HAS_LAME
    file.deleteFile();
    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;

    lame_t lame = lame_init();
    if (!lame)
        return false;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    lame_set_in_samplerate(lame, static_cast<int>(sampleRate));
    lame_set_num_channels(lame, numChannels);
    lame_set_brate(lame, bitrate);
    lame_set_quality(lame, 2);  // near-best quality
    lame_set_mode(lame, numChannels == 1 ? MONO : JOINT_STEREO);

    if (lame_init_params(lame) < 0) {
        lame_close(lame);
        return false;
    }

    // Encode in chunks
    const int chunkSize = 8192;
    // MP3 buffer needs 1.25 * chunkSize + 7200 bytes
    std::vector<unsigned char> mp3Buffer(static_cast<size_t>(chunkSize * 1.25 + 7200));

    // Interleave audio data for LAME
    std::vector<float> interleaved(static_cast<size_t>(chunkSize * numChannels));

    int samplesProcessed = 0;
    while (samplesProcessed < numSamples) {
        int samplesThisChunk = std::min(chunkSize, numSamples - samplesProcessed);

        for (int s = 0; s < samplesThisChunk; ++s) {
            for (int ch = 0; ch < numChannels; ++ch) {
                interleaved[static_cast<size_t>(s * numChannels + ch)] =
                    buffer.getSample(ch, samplesProcessed + s);
            }
        }

        int bytesEncoded = lame_encode_buffer_interleaved_ieee_float(
            lame,
            interleaved.data(),
            samplesThisChunk,
            mp3Buffer.data(),
            static_cast<int>(mp3Buffer.size()));

        if (bytesEncoded < 0) {
            lame_close(lame);
            return false;
        }

        if (bytesEncoded > 0) {
            stream.write(mp3Buffer.data(), static_cast<size_t>(bytesEncoded));
        }

        samplesProcessed += samplesThisChunk;
    }

    // Flush remaining MP3 data
    int flushBytes = lame_encode_flush(lame, mp3Buffer.data(),
                                        static_cast<int>(mp3Buffer.size()));
    if (flushBytes > 0) {
        stream.write(mp3Buffer.data(), static_cast<size_t>(flushBytes));
    }

    lame_close(lame);
    return true;
#else
    juce::ignoreUnused(file, buffer, sampleRate, bitrate);
    return false;
#endif
}

// ---------------------------------------------------------------------------
// Export full mix
// ---------------------------------------------------------------------------

bool AudioExporter::exportMix(const ExportOptions& options, ProgressCallback progress)
{
    if (options.outputPath.empty() || !isFormatSupported(options.format))
        return false;
    if (!engine_.isInitialised())
        return false;

    const double totalBeats = resolveTotalBeats(options.totalBeats);
    if (totalBeats <= 0.0)
        return false;  // nothing to render

    auto& graph = engine_.getAudioGraph();
    auto buffer = graph.renderOffline(totalBeats, kTailSeconds, {}, progress);
    if (buffer.getNumSamples() == 0)
        return false;

    const double sampleRate = graph.getAudioConfig().sampleRate;
    juce::File outputFile(juce::String::fromUTF8(options.outputPath.c_str()));
    outputFile.getParentDirectory().createDirectory();

    switch (options.format) {
        case ExportFormat::WAV_16: return writeWav(outputFile, buffer, sampleRate, 16);
        case ExportFormat::WAV_24: return writeWav(outputFile, buffer, sampleRate, 24);
        case ExportFormat::FLAC:   return writeFlac(outputFile, buffer, sampleRate);
        case ExportFormat::MP3:    return writeMp3(outputFile, buffer, sampleRate, options.mp3Bitrate);
    }

    return false;
}

// ---------------------------------------------------------------------------
// Export stems: one 24-bit WAV per track, each rendered with that track soloed
// ---------------------------------------------------------------------------

bool AudioExporter::exportStems(const StemExportOptions& options, ProgressCallback progress)
{
    if (options.outputDir.empty() || !engine_.isInitialised())
        return false;

    const double totalBeats = resolveTotalBeats(options.totalBeats);
    if (totalBeats <= 0.0)
        return false;

    juce::File outputDir(juce::String::fromUTF8(options.outputDir.c_str()));
    if (!outputDir.isDirectory() && !outputDir.createDirectory())
        return false;

    auto& graph = engine_.getAudioGraph();
    const auto trackIds = graph.getTrackIds();
    if (trackIds.empty())
        return false;

    const double sampleRate = graph.getAudioConfig().sampleRate;
    const float numTracks = static_cast<float>(trackIds.size());
    juce::StringArray usedNames;

    for (size_t i = 0; i < trackIds.size(); ++i) {
        const auto& trackId = trackIds[i];

        auto trackProgress = [&](float fraction) {
            if (progress)
                progress((static_cast<float>(i) + fraction) / numTracks);
        };
        auto buffer = graph.renderOffline(totalBeats, kTailSeconds, trackId, trackProgress);
        if (buffer.getNumSamples() == 0)
            return false;

        auto nameIt = options.trackNames.find(trackId);
        auto baseName = safeFileName(nameIt != options.trackNames.end()
                                         ? juce::String::fromUTF8(nameIt->second.c_str())
                                         : juce::String(trackId));
        // Two tracks may share a display name; never overwrite one stem with another.
        auto fileName = baseName;
        for (int suffix = 2; usedNames.contains(fileName, true); ++suffix)
            fileName = baseName + " " + juce::String(suffix);
        usedNames.add(fileName);

        if (!writeWav(outputDir.getChildFile(fileName + ".wav"), buffer, sampleRate, 24))
            return false;
    }

    return true;
}

} // namespace calliope
