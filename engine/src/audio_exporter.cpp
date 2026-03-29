#include "calliope/audio_exporter.h"
#include "calliope/engine.h"
#include "calliope/audio_graph.h"
#include "calliope/transport.h"

#ifdef CALLIOPE_HAS_LAME
#include <lame/lame.h>
#endif

namespace calliope {

AudioExporter::AudioExporter(Engine& engine) : engine_(engine) {}

// ---------------------------------------------------------------------------
// MIDI JSON parsing — uses exact camelCase field names from JS contract
// ---------------------------------------------------------------------------

std::vector<MidiEventData> AudioExporter::parseMidiEventsJson(const juce::String& jsonString)
{
    std::vector<MidiEventData> events;

    auto parsed = juce::JSON::parse(jsonString);
    if (!parsed.isArray())
        return events;

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
// Offline bounce — drives AudioProcessorGraph.processBlock() in a loop
// ---------------------------------------------------------------------------

juce::AudioBuffer<float> AudioExporter::offlineBounce(
    double totalBeats, double sampleRate, int bufferSize,
    const std::vector<MidiEventData>& midiEvents,
    ProgressCallback progress)
{
    auto& graph = engine_.getAudioGraph();
    auto& transport = graph.getTransport();

    // Use project sample rate if not specified
    if (sampleRate <= 0.0)
        sampleRate = graph.getAudioConfig().sampleRate;

    double bpm = transport.getBpm();

    // Calculate total samples: totalBeats / bpm * 60 * sampleRate
    // Add a small tail (1 second) for reverb/delay tails
    double totalSeconds = (totalBeats / bpm) * 60.0 + 1.0;
    int64_t totalSamples = static_cast<int64_t>(totalSeconds * sampleRate);

    // Prepare output buffer
    juce::AudioBuffer<float> output(2, static_cast<int>(totalSamples));
    output.clear();

    // Stop transport and reset position
    transport.stop();
    transport.setSampleRate(sampleRate);

    // Prepare graph for offline rendering
    graph.getTransport().stop();

    // Prepare all processors in the graph
    // We use the graph's processBlock directly — the graph is already set up
    // We need to re-prepare at our offline sample rate / buffer size
    // Note: We temporarily disconnect from audio device by just not using it

    // Sort MIDI events by beat position for efficient scheduling
    auto sortedEvents = midiEvents;
    std::sort(sortedEvents.begin(), sortedEvents.end(),
              [](const MidiEventData& a, const MidiEventData& b) {
                  return a.beatPosition < b.beatPosition;
              });

    // Track note-off scheduling
    struct PendingNoteOff {
        double beatPosition;
        int noteNumber;
        int channel; // 1-based MIDI channel
    };
    std::vector<PendingNoteOff> pendingNoteOffs;

    // Map trackId to MIDI channel for instrument routing
    auto channelForTrack = [](const std::string& trackId) -> int {
        if (trackId == "polySynth" || trackId == "polysynth") return 1;
        if (trackId == "bassSynth" || trackId == "basssynth") return 2;
        if (trackId == "drumMachine") return 10;
        return 1; // default
    };

    // Process blocks
    juce::AudioBuffer<float> blockBuffer(2, bufferSize);
    juce::MidiBuffer midiBuffer;
    int64_t samplesRendered = 0;
    size_t nextEventIndex = 0;

    // Start transport playing for proper time advancement
    transport.play();

    while (samplesRendered < totalSamples) {
        int samplesThisBlock = static_cast<int>(
            std::min(static_cast<int64_t>(bufferSize), totalSamples - samplesRendered));

        blockBuffer.clear();
        midiBuffer.clear();

        // Calculate beat range for this block
        double blockStartBeat = (static_cast<double>(samplesRendered) / sampleRate) * (bpm / 60.0);
        double blockEndBeat = (static_cast<double>(samplesRendered + samplesThisBlock) / sampleRate) * (bpm / 60.0);

        // Schedule note-on events that fall within this block
        while (nextEventIndex < sortedEvents.size() &&
               sortedEvents[nextEventIndex].beatPosition < blockEndBeat) {
            const auto& evt = sortedEvents[nextEventIndex];
            if (evt.beatPosition >= blockStartBeat) {
                // Calculate sample offset within this block
                double eventTimeSec = (evt.beatPosition / bpm) * 60.0;
                int sampleOffset = static_cast<int>(eventTimeSec * sampleRate) - static_cast<int>(samplesRendered);
                sampleOffset = juce::jlimit(0, samplesThisBlock - 1, sampleOffset);

                int channel = channelForTrack(evt.trackId);
                auto noteOn = juce::MidiMessage::noteOn(channel, evt.noteNumber,
                    static_cast<juce::uint8>(juce::jlimit(1, 127, evt.velocity)));
                midiBuffer.addEvent(noteOn, sampleOffset);

                // Schedule note-off
                pendingNoteOffs.push_back({
                    evt.beatPosition + evt.durationBeats,
                    evt.noteNumber,
                    channel
                });
            }
            nextEventIndex++;
        }

        // Schedule note-off events that fall within this block
        for (auto it = pendingNoteOffs.begin(); it != pendingNoteOffs.end();) {
            if (it->beatPosition < blockEndBeat) {
                double offTimeSec = (it->beatPosition / bpm) * 60.0;
                int sampleOffset = static_cast<int>(offTimeSec * sampleRate) - static_cast<int>(samplesRendered);
                sampleOffset = juce::jlimit(0, samplesThisBlock - 1, sampleOffset);

                auto noteOff = juce::MidiMessage::noteOff(it->channel, it->noteNumber);
                midiBuffer.addEvent(noteOff, sampleOffset);
                it = pendingNoteOffs.erase(it);
            } else {
                ++it;
            }
        }

        // Advance transport position manually
        transport.advancePosition(samplesThisBlock);

        // Process the graph
        blockBuffer.setSize(2, samplesThisBlock, false, false, true);
        blockBuffer.clear();
        graph.getMasterBus().processBlock(blockBuffer, midiBuffer);

        // Copy processed samples to output
        for (int ch = 0; ch < 2; ++ch) {
            output.copyFrom(ch, static_cast<int>(samplesRendered),
                           blockBuffer, ch, 0, samplesThisBlock);
        }

        samplesRendered += samplesThisBlock;

        // Report progress
        if (progress) {
            float pct = static_cast<float>(samplesRendered) / static_cast<float>(totalSamples);
            progress(std::min(pct, 1.0f));
        }
    }

    // Stop transport after rendering
    transport.stop();

    return output;
}

// ---------------------------------------------------------------------------
// Single-track offline bounce (for stem export)
// ---------------------------------------------------------------------------

juce::AudioBuffer<float> AudioExporter::offlineBounceSingleTrack(
    const juce::String& trackId, double totalBeats, double sampleRate,
    int bufferSize, const std::vector<MidiEventData>& midiEvents)
{
    // Filter MIDI events for this track only
    std::vector<MidiEventData> trackEvents;
    for (const auto& evt : midiEvents) {
        juce::String evtTrackId(evt.trackId);
        if (evtTrackId.equalsIgnoreCase(trackId))
            trackEvents.push_back(evt);
    }

    // For stem export, we render only events for this track
    // The offline bounce will still route through the graph,
    // but only this track's instrument will receive MIDI
    return offlineBounce(totalBeats, sampleRate, bufferSize, trackEvents, nullptr);
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
        wavFormat.createWriterFor(stream.release(), sampleRate,
                                  static_cast<unsigned int>(buffer.getNumChannels()),
                                  bitDepth, {}, 0));
    if (!writer)
        return false;

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
        flacFormat.createWriterFor(stream.release(), sampleRate,
                                   static_cast<unsigned int>(buffer.getNumChannels()),
                                   16, {}, 0));
    if (!writer)
        return false;

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

        // Interleave channels
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
    if (options.outputPath.empty())
        return false;

    double sr = options.sampleRate > 0.0
        ? options.sampleRate
        : engine_.getAudioGraph().getAudioConfig().sampleRate;

    double totalBeats = options.totalBeats > 0.0 ? options.totalBeats : 4.0;

    // Perform offline bounce
    auto buffer = offlineBounce(totalBeats, sr, 512, options.midiEvents, progress);

    if (buffer.getNumSamples() == 0)
        return false;

    juce::File outputFile(options.outputPath);

    switch (options.format) {
        case ExportFormat::WAV_16:
            return writeWav(outputFile, buffer, sr, 16);
        case ExportFormat::WAV_24:
            return writeWav(outputFile, buffer, sr, 24);
        case ExportFormat::FLAC:
            return writeFlac(outputFile, buffer, sr);
        case ExportFormat::MP3:
            return writeMp3(outputFile, buffer, sr, options.mp3Bitrate);
    }

    return false;
}

// ---------------------------------------------------------------------------
// Export stems
// ---------------------------------------------------------------------------

bool AudioExporter::exportStems(const StemExportOptions& options, ProgressCallback progress)
{
    if (options.outputDir.empty())
        return false;

    juce::File outputDir(options.outputDir);
    if (!outputDir.exists())
        outputDir.createDirectory();

    double sr = options.sampleRate > 0.0
        ? options.sampleRate
        : engine_.getAudioGraph().getAudioConfig().sampleRate;

    double totalBeats = options.totalBeats > 0.0 ? options.totalBeats : 4.0;

    // Track names to export
    const juce::String trackIds[] = { "polysynth", "basssynth", "drumMachine" };
    const juce::String trackNames[] = { "polySynth", "bassSynth", "drumMachine" };
    int numTracks = 3;

    for (int i = 0; i < numTracks; ++i) {
        auto buffer = offlineBounceSingleTrack(trackIds[i], totalBeats, sr, 512,
                                                options.midiEvents);

        juce::File stemFile = outputDir.getChildFile(trackNames[i] + ".wav");
        if (!writeWav(stemFile, buffer, sr, 24))
            return false;

        if (progress) {
            progress(static_cast<float>(i + 1) / static_cast<float>(numTracks));
        }
    }

    return true;
}

} // namespace calliope
