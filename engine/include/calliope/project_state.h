#pragma once
#include <juce_core/juce_core.h>
#include "calliope/clip_scheduler.h"
#include <vector>

namespace calliope {

// Forward declarations
class Engine;

// A complete, replayable description of the engine: Engine::applyProjectState
// rebuilds exactly this from an empty project.
//
// Instrument, mixer and effect settings are not modelled field by field. They
// live in `parameters`, keyed by the same IDs the ParameterRegistry exposes
// ("track.{id}.filterCutoff", "effects.{id}.0.threshold", ...), so a new
// parameter is saved and restored without touching this class.
class ProjectState {
public:
    static constexpr int kFormatVersion = 2;

    struct TransportData {
        juce::String state = "stopped";
        double bpm = 120.0;
        int timeSigNumerator = 4;
        int timeSigDenominator = 4;
        bool looping = false;
        double loopStartBeat = 0.0;
        double loopEndBeat = 0.0;
    };

    struct MetronomeData {
        bool enabled = true;
        float volume = 0.7f;
    };

    struct MasterBusData {
        float volume = 1.0f;
    };

    struct AudioConfigData {
        double sampleRate = 44100.0;
        int bufferSize = 512;
    };

    struct TrackData {
        juce::String id;
        juce::String instrument;   // one of InstrumentTypes
    };

    struct EffectSlotData {
        juce::String effectType;   // factory key: "eq", "compressor", "reverb", "delay", "limiter"
        bool bypassed = false;
    };

    struct InsertChainData {
        juce::String trackId;      // a track ID or "master"
        std::vector<EffectSlotData> effects;
    };

    int formatVersion = kFormatVersion;
    TransportData transport;
    MetronomeData metronome;
    MasterBusData masterBus;
    AudioConfigData audioConfig;
    std::vector<TrackData> tracks;
    std::vector<InsertChainData> effectChains;
    std::vector<MidiClipData> clips;
    juce::var parameters;          // object: parameter ID -> value

    // Serialize to JSON string
    juce::String toJson() const;

    // Deserialize from JSON string. Returns false on parse failure.
    bool fromJson(const juce::String& jsonString);

    // Populate from live Engine state
    void snapshotFromEngine(const Engine& engine);
};

} // namespace calliope
