#pragma once
#include <juce_core/juce_core.h>

namespace calliope {

// Forward declarations
class Engine;

class ProjectState {
public:
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

    TransportData transport;
    MetronomeData metronome;
    MasterBusData masterBus;
    AudioConfigData audioConfig;

    // Serialize to JSON string
    juce::String toJson() const;

    // Deserialize from JSON string. Returns false on parse failure.
    bool fromJson(const juce::String& jsonString);

    // Populate from live Engine state
    void snapshotFromEngine(const Engine& engine);
};

} // namespace calliope
