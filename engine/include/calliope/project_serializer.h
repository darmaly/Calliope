#pragma once
#include <juce_core/juce_core.h>
#include "calliope/project_state.h"

namespace calliope {

class ProjectSerializer {
public:
    static constexpr int FORMAT_VERSION = 1;
    static constexpr const char* APP_VERSION = "1.0.0";

    // Save ProjectState to file as versioned JSON envelope.
    // Returns true on success.
    static bool saveToFile(const juce::String& filePath, const ProjectState& state);

    // Load ProjectState from file. Returns true on success, populates state.
    static bool loadFromFile(const juce::String& filePath, ProjectState& state);

    // Get the format version from a project file without full parse.
    static int getFileVersion(const juce::String& filePath);

private:
    // Wrap state JSON in version envelope
    static juce::String createEnvelope(const juce::String& stateJson);

    // Extract state JSON from version envelope
    static juce::String extractFromEnvelope(const juce::String& envelopeJson, int& version);
};

} // namespace calliope
