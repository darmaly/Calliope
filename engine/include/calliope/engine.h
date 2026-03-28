#pragma once
#include <string>
#include <vector>
#include <memory>

namespace calliope {

class Engine {
public:
    static std::string getJuceVersion();
    static std::vector<std::string> getAudioDevices();

    // Test tone control
    static bool startTestTone(double frequency, int deviceIndex = -1);
    static void stopTestTone();

private:
    Engine() = default;
};

} // namespace calliope
