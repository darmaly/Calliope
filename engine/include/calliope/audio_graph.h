#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "calliope/transport.h"
#include "calliope/master_bus.h"
#include "calliope/metronome.h"
#include "calliope/lock_free_queue.h"
#include "calliope/instruments/poly_synth.h"
#include "calliope/instruments/bass_synth.h"
#include "calliope/instruments/drum_machine.h"
#include "calliope/insert_chain_processor.h"
#include <memory>

namespace calliope {

struct AudioConfig {
    double sampleRate = 44100.0;
    int bufferSize = 512;
};

class AudioGraph {
public:
    AudioGraph();
    ~AudioGraph();

    // Lifecycle
    bool initialise(double sampleRate, int bufferSize);
    void shutdown();
    bool isInitialised() const;

    // Buffer size change (requires device restart)
    bool setBufferSize(int newBufferSize);

    // Access to sub-components
    Transport& getTransport();
    MasterBusProcessor& getMasterBus();
    MetronomeProcessor& getMetronome();
    AudioConfig getAudioConfig() const;

    // Instrument access (Phase 4)
    PolySynthProcessor& getPolySynth();
    BassSynthProcessor& getBassSynth();
    DrumMachineProcessor& getDrumMachine();

    // Insert chain access (Phase 5)
    InsertChainProcessor& getInsertChainProcessor(const juce::String& trackId);
    InsertChain& getInsertChain(const juce::String& trackId);

private:
    juce::AudioDeviceManager deviceManager_;
    juce::AudioProcessorPlayer player_;
    juce::AudioProcessorGraph graph_;
    Transport transport_;

    // Owned processors (raw pointers -- graph owns the unique_ptrs)
    MasterBusProcessor* masterBusPtr_ = nullptr;
    MetronomeProcessor* metronomePtr_ = nullptr;
    PolySynthProcessor* polySynthPtr_ = nullptr;
    BassSynthProcessor* bassSynthPtr_ = nullptr;
    DrumMachineProcessor* drumMachinePtr_ = nullptr;

    // Insert chain processors (raw pointers -- graph owns the unique_ptrs)
    InsertChainProcessor* polySynthChainPtr_ = nullptr;
    InsertChainProcessor* bassSynthChainPtr_ = nullptr;
    InsertChainProcessor* drumMachineChainPtr_ = nullptr;
    InsertChainProcessor* masterChainPtr_ = nullptr;

    // Graph node IDs
    juce::AudioProcessorGraph::Node::Ptr outputNode_;
    juce::AudioProcessorGraph::Node::Ptr masterNode_;
    juce::AudioProcessorGraph::Node::Ptr metronomeNode_;
    juce::AudioProcessorGraph::Node::Ptr polySynthNode_;
    juce::AudioProcessorGraph::Node::Ptr bassSynthNode_;
    juce::AudioProcessorGraph::Node::Ptr drumMachineNode_;
    juce::AudioProcessorGraph::Node::Ptr polySynthChainNode_;
    juce::AudioProcessorGraph::Node::Ptr bassSynthChainNode_;
    juce::AudioProcessorGraph::Node::Ptr drumMachineChainNode_;
    juce::AudioProcessorGraph::Node::Ptr masterChainNode_;

    AudioConfig currentConfig_;
    bool initialised_ = false;

    // Custom AudioIODeviceCallback that advances transport
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
