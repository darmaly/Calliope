#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include "calliope/audio_graph.h"
#include "calliope/command_dispatcher.h"
#include "calliope/parameter_registry.h"
#include "calliope/project_state.h"
#include "calliope/audio_exporter.h"

namespace calliope {

class Engine {
public:
    // Singleton access
    static Engine& getInstance();

    // Delete copy/move
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    static std::string getJuceVersion();
    static std::vector<std::string> getAudioDevices();

    // Legacy test tone (Phase 1 proof of life)
    static bool startTestTone(double frequency, int deviceIndex = -1);
    static void stopTestTone();

    // Lifecycle. initialise() is idempotent; shutdown() allows a later initialise().
    bool initialise(double sampleRate = 44100.0, int bufferSize = 512);
    void shutdown();
    bool isInitialised() const;

    // Sub-component access (engine must be initialised)
    AudioGraph& getAudioGraph();
    Transport& getTransport();
    ClipScheduler& getClipScheduler();

    // --- Tracks ---
    // Creates the track's instrument and channel, and registers its parameters
    // as "track.{trackId}.{name}".
    bool addTrack(const std::string& trackId, const std::string& instrumentType);
    bool removeTrack(const std::string& trackId);

    // Back to an empty project: no tracks, clips, effects or undo history,
    // and default transport, metronome and master settings.
    void resetProject();

    // Insert chain of a track or of "master". Null when the ID is unknown.
    InsertChain* findInsertChain(const juce::String& trackId);

    // Dynamic effect parameter registration/unregistration
    void registerEffectParameters(const juce::String& trackId, int slotIndex, juce::AudioProcessor* effect);
    void unregisterEffectParameters(const juce::String& trackId, int slotIndex);

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

    // Project state: a complete, replayable description of the engine.
    ProjectState getProjectState() const;
    bool applyProjectState(const ProjectState& state);

    // Metering (delegates to AudioGraph)
    AudioGraph::AllMeterLevels getMeterLevels() const;

    // Audio export
    AudioExporter& getAudioExporter();

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

    std::mutex initMutex_;
    std::atomic<bool> initialised_{false};
    std::unique_ptr<AudioGraph> audioGraph_;
    std::unique_ptr<AudioExporter> audioExporter_;
    CommandDispatcher dispatcher_;
    ParameterRegistry paramRegistry_;

    void registerGlobalParameters();
    void registerTrackParameters(const std::string& trackId);
    void unregisterTrackParameters(const std::string& trackId);
};

} // namespace calliope
