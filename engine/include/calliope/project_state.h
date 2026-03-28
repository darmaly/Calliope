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

    struct PolySynthData {
        int osc1Waveform = 0;
        int osc2Waveform = 2;
        float oscMix = 0.5f;
        float osc2Detune = 0.0f;
        float filterCutoff = 5000.0f;
        float filterResonance = 0.1f;
        float filterEnvAmount = 0.3f;
        float ampAttack = 0.01f;
        float ampDecay = 0.1f;
        float ampSustain = 0.8f;
        float ampRelease = 0.3f;
        float filterAttack = 0.01f;
        float filterDecay = 0.2f;
        float filterSustain = 0.5f;
        float filterRelease = 0.3f;
        float lfoRate = 1.0f;
        float lfoDepth = 0.0f;
        int lfoTarget = 1;
        float masterGain = 0.7f;
    };

    struct BassSynthData {
        int oscWaveform = 0;
        float subOscMix = 0.5f;
        int subOscOctave = -1;
        float filterCutoff = 800.0f;
        float filterResonance = 0.2f;
        float filterEnvAmount = 0.4f;
        float ampAttack = 0.005f;
        float ampDecay = 0.15f;
        float ampSustain = 0.9f;
        float ampRelease = 0.2f;
        float filterAttack = 0.005f;
        float filterDecay = 0.3f;
        float filterSustain = 0.3f;
        float filterRelease = 0.2f;
        float masterGain = 0.8f;
    };

    struct DrumMachineData {
        float volume = 0.8f;
        std::vector<juce::String> padNames;
    };

    struct EffectSlotData {
        juce::String effectType;
        bool bypassed = false;
        juce::var parameters;  // JSON object with parameter key-value pairs
    };

    struct InsertChainData {
        juce::String trackId;
        std::vector<EffectSlotData> effects;
    };

    TransportData transport;
    MetronomeData metronome;
    MasterBusData masterBus;
    AudioConfigData audioConfig;
    PolySynthData polySynth;
    BassSynthData bassSynth;
    DrumMachineData drumMachine;
    std::vector<InsertChainData> effectChains;

    // Serialize to JSON string
    juce::String toJson() const;

    // Deserialize from JSON string. Returns false on parse failure.
    bool fromJson(const juce::String& jsonString);

    // Populate from live Engine state
    void snapshotFromEngine(const Engine& engine);
};

} // namespace calliope
