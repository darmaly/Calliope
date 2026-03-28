#pragma once
#include <string>
#include <vector>
#include <memory>
#include "calliope/audio_graph.h"
#include "calliope/command_dispatcher.h"
#include "calliope/parameter_registry.h"
#include "calliope/project_state.h"

namespace calliope {

class Engine {
public:
    // Singleton access
    static Engine& getInstance();

    // Delete copy/move
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Preserved from Phase 1 for backward compatibility
    static std::string getJuceVersion();
    static std::vector<std::string> getAudioDevices();

    // Legacy test tone (preserved for Phase 1 bridge compatibility)
    static bool startTestTone(double frequency, int deviceIndex = -1);
    static void stopTestTone();

    // New Phase 2 API
    bool initialise(double sampleRate = 44100.0, int bufferSize = 512);
    void shutdown();
    bool isInitialised() const;

    // Sub-component access
    AudioGraph& getAudioGraph();
    Transport& getTransport();

    // Instrument access (Phase 4 convenience)
    PolySynthProcessor& getPolySynth();
    BassSynthProcessor& getBassSynth();
    DrumMachineProcessor& getDrumMachine();

    // Convenience wrappers (delegate to AudioGraph/Transport)
    void transportPlay();
    void transportStop();
    void transportPause();
    void setBpm(double bpm);
    void setTimeSignature(int numerator, int denominator);
    void setLoopRegion(double startBeat, double endBeat, bool enabled);
    bool setBufferSize(int bufferSize);
    void setMetronomeEnabled(bool enabled);
    void setMetronomeVolume(float volume);

    // Command dispatcher and parameter registry
    CommandDispatcher& getCommandDispatcher();
    ParameterRegistry& getParameterRegistry();
    ProjectState getProjectState() const;

    // State queries
    struct TransportStateInfo {
        std::string state;  // "stopped", "playing", "paused"
        double bpm;
        int timeSigNumerator;
        int timeSigDenominator;
        int64_t samplePosition;
        double ppqPosition;
        bool looping;
        double loopStartBeat;
        double loopEndBeat;
    };
    TransportStateInfo getTransportState() const;

    struct AudioConfigInfo {
        double sampleRate;
        int bufferSize;
        bool initialised;
    };
    AudioConfigInfo getAudioConfig() const;

private:
    Engine() = default;
    ~Engine();

    std::unique_ptr<AudioGraph> audioGraph_;
    CommandDispatcher dispatcher_;
    ParameterRegistry paramRegistry_;

    void registerParameters();
};

} // namespace calliope
