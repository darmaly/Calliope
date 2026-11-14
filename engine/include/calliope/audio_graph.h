#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "calliope/transport.h"
#include "calliope/master_bus.h"
#include "calliope/metronome.h"
#include "calliope/lock_free_queue.h"
#include "calliope/midi_sink.h"
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/drum_machine.h"
#include "calliope/insert_chain_processor.h"
#include "calliope/clip_scheduler.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace calliope {

struct AudioConfig {
    double sampleRate = 44100.0;
    int bufferSize = 512;
};

// Instrument type names accepted by AudioGraph::addTrack.
namespace InstrumentTypes {
    inline const char* const polySynth = "polysynth";
    inline const char* const bassSynth = "basssynth";
    inline const char* const drumMachine = "drumMachine";
}

// Owns the audio device and the processing graph:
//
//   track instrument -> track channel (inserts, gain, pan, mute) --+
//   track instrument -> track channel ---------------------------- +-> master bus -> master inserts -> output
//   metronome ---------------------------------------------------- +
//
// Tracks are created and removed at runtime. Every public method is intended
// for control threads; the audio thread only sees the graph and the scheduler.
class AudioGraph {
public:
    AudioGraph();
    ~AudioGraph();

    // Lifecycle
    bool initialise(double sampleRate, int bufferSize);
    void shutdown();
    bool isInitialised() const;

    // Reopens the audio device with a new buffer size. Tracks are kept.
    bool setBufferSize(int newBufferSize);

    // Access to sub-components
    Transport& getTransport();
    MasterBusProcessor& getMasterBus();
    MetronomeProcessor& getMetronome();
    AudioConfig getAudioConfig() const;
    ClipScheduler& getClipScheduler();

    // --- Tracks ---
    // instrumentType is one of InstrumentTypes. Fails on a duplicate ID,
    // the reserved ID "master", or an unknown instrument.
    bool addTrack(const std::string& trackId, const std::string& instrumentType);
    bool removeTrack(const std::string& trackId);
    void clearTracks();
    bool hasTrack(const std::string& trackId) const;
    std::vector<std::string> getTrackIds() const;            // in creation order
    std::string getTrackInstrumentType(const std::string& trackId) const;

    // Null when the track does not exist.
    juce::AudioProcessor* getTrackInstrument(const std::string& trackId) const;
    MidiSink* getTrackMidiSink(const std::string& trackId) const;
    InsertChainProcessor* getTrackChannel(const std::string& trackId) const;

    void setTrackSolo(const std::string& trackId, bool solo);
    bool isTrackSoloed(const std::string& trackId) const;

    // Insert chains. trackId may be a track or "master". Null when unknown.
    InsertChainProcessor* findInsertChainProcessor(const juce::String& trackId) const;
    InsertChain* findInsertChain(const juce::String& trackId) const;

    // --- Metering ---
    struct AllMeterLevels {
        struct TrackMeter {
            std::string trackId;
            float rmsLeft = 0.f, rmsRight = 0.f;
            float peakLeft = 0.f, peakRight = 0.f;
        };
        std::vector<TrackMeter> tracks;
        TrackMeter master;
    };
    // Never blocks: returns the master meter only if the graph is busy.
    AllMeterLevels getMeterLevels() const;

    // --- Offline rendering ---
    // Renders the arrangement from beat 0 through the same graph the device
    // uses, with the device detached and the metronome silenced.
    // soloTrackId, when non-empty, renders that track alone (for stems).
    // The returned buffer is at getAudioConfig().sampleRate.
    juce::AudioBuffer<float> renderOffline(double totalBeats, double tailSeconds,
                                           const std::string& soloTrackId,
                                           const std::function<void(float)>& progress);

private:
    struct Track {
        std::string id;
        std::string instrumentType;
        juce::AudioProcessorGraph::Node::Ptr instrumentNode;
        juce::AudioProcessorGraph::Node::Ptr channelNode;
        juce::AudioProcessor* instrument = nullptr;
        MidiSink* sink = nullptr;
        InsertChainProcessor* channel = nullptr;
        bool solo = false;
    };

    juce::AudioDeviceManager deviceManager_;
    juce::AudioProcessorPlayer player_;
    juce::AudioProcessorGraph graph_;
    Transport transport_;
    ClipScheduler clipScheduler_;

    // Raw pointers -- the graph owns the processors
    MasterBusProcessor* masterBusPtr_ = nullptr;
    MetronomeProcessor* metronomePtr_ = nullptr;
    InsertChainProcessor* masterChainPtr_ = nullptr;

    juce::AudioProcessorGraph::Node::Ptr outputNode_;
    juce::AudioProcessorGraph::Node::Ptr masterNode_;
    juce::AudioProcessorGraph::Node::Ptr metronomeNode_;
    juce::AudioProcessorGraph::Node::Ptr masterChainNode_;

    // Guards tracks_ and graph topology changes.
    mutable std::recursive_mutex graphMutex_;
    std::vector<Track> tracks_;

    AudioConfig currentConfig_;
    bool initialised_ = false;

    const Track* findTrack(const std::string& trackId) const;
    Track* findTrack(const std::string& trackId);
    void applySoloState();  // graphMutex_ must be held
    void rebuildGraphNow(); // graphMutex_ must be held

    // Custom AudioIODeviceCallback that drives the scheduler and transport
    class GraphCallback : public juce::AudioIODeviceCallback {
    public:
        GraphCallback(AudioGraph& owner);
        void audioDeviceIOCallbackWithContext(
            const float* const* inputChannelData, int numInputChannels,
            float* const* outputChannelData, int numOutputChannels,
            int numSamples,
            const juce::AudioIODeviceCallbackContext& context) override;
        void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
        void audioDeviceStopped() override;
    private:
        AudioGraph& owner_;
    };

    std::unique_ptr<GraphCallback> graphCallback_;
};

} // namespace calliope
