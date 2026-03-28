#include "calliope/engine.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <cmath>
#include <atomic>
#include <mutex>

namespace calliope {

// Simple sine wave audio callback
class TestToneCallback : public juce::AudioIODeviceCallback {
public:
    TestToneCallback(double freq) : frequency(freq) {}

    void audioDeviceIOCallbackWithContext(
        const float* const* /*inputChannelData*/,
        int /*numInputChannels*/,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        const double currentFreq = frequency.load();

        for (int sample = 0; sample < numSamples; ++sample) {
            float value = static_cast<float>(
                std::sin(2.0 * juce::MathConstants<double>::pi * phase) * 0.25
            );
            phase += currentFreq / currentSampleRate;
            if (phase >= 1.0)
                phase -= 1.0;

            for (int channel = 0; channel < numOutputChannels; ++channel) {
                outputChannelData[channel][sample] = value;
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override {
        currentSampleRate = device->getCurrentSampleRate();
        phase = 0.0;
    }

    void audioDeviceStopped() override {
        phase = 0.0;
    }

private:
    std::atomic<double> frequency{440.0};
    double currentSampleRate = 44100.0;
    double phase = 0.0;
};

// Static state for test tone
static std::mutex toneMutex;
static std::unique_ptr<juce::AudioDeviceManager> deviceManager;
static std::unique_ptr<TestToneCallback> toneCallback;

bool Engine::startTestTone(double frequency, int /*deviceIndex*/) {
    std::lock_guard<std::mutex> lock(toneMutex);

    // Ensure message manager exists
    juce::MessageManager::getInstance();

    // Stop any existing tone first
    if (deviceManager && toneCallback) {
        deviceManager->removeAudioCallback(toneCallback.get());
        deviceManager->closeAudioDevice();
        toneCallback.reset();
        deviceManager.reset();
    }

    deviceManager = std::make_unique<juce::AudioDeviceManager>();
    toneCallback = std::make_unique<TestToneCallback>(frequency);

    // Initialize with default output device, 0 inputs, 2 outputs
    auto error = deviceManager->initialise(0, 2, nullptr, true);
    if (error.isNotEmpty()) {
        deviceManager.reset();
        toneCallback.reset();
        return false;
    }

    deviceManager->addAudioCallback(toneCallback.get());
    return true;
}

void Engine::stopTestTone() {
    std::lock_guard<std::mutex> lock(toneMutex);

    if (deviceManager && toneCallback) {
        deviceManager->removeAudioCallback(toneCallback.get());
        deviceManager->closeAudioDevice();
        toneCallback.reset();
        deviceManager.reset();
    }
}

} // namespace calliope
