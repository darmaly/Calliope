#include "calliope/engine.h"
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace calliope {

std::string Engine::getJuceVersion() {
    return juce::SystemStats::getJUCEVersion().toStdString();
}

std::vector<std::string> Engine::getAudioDevices() {
    // Ensure message manager exists (required for AudioDeviceManager)
    juce::MessageManager::getInstance();

    juce::AudioDeviceManager manager;
    std::vector<std::string> devices;

    for (auto* type : manager.getAvailableDeviceTypes()) {
        type->scanForDevices();
        for (auto& name : type->getDeviceNames()) {
            devices.push_back(type->getTypeName().toStdString()
                + ": " + name.toStdString());
        }
    }

    return devices;
}

} // namespace calliope
