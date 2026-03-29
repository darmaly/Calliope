#include "calliope/project_serializer.h"

namespace calliope {

juce::String ProjectSerializer::createEnvelope(const juce::String& stateJson)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("version", FORMAT_VERSION);
    root->setProperty("appVersion", juce::String(APP_VERSION));
    root->setProperty("savedAt", juce::Time::getCurrentTime().toISO8601(true));

    // Parse the state JSON back into a var so it nests properly
    auto stateVar = juce::JSON::parse(stateJson);
    root->setProperty("data", stateVar);

    return juce::JSON::toString(juce::var(root));
}

juce::String ProjectSerializer::extractFromEnvelope(const juce::String& envelopeJson, int& version)
{
    auto parsed = juce::JSON::parse(envelopeJson);
    if (!parsed.isObject())
    {
        version = -1;
        return {};
    }

    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
    {
        version = -1;
        return {};
    }

    version = static_cast<int>(root->getProperty("version"));

    auto dataVar = root->getProperty("data");
    if (dataVar.isVoid())
    {
        return {};
    }

    return juce::JSON::toString(dataVar);
}

bool ProjectSerializer::saveToFile(const juce::String& filePath, const ProjectState& state)
{
    auto stateJson = state.toJson();
    auto envelope = createEnvelope(stateJson);

    juce::File file(filePath);

    // Ensure parent directory exists
    file.getParentDirectory().createDirectory();

    return file.replaceWithText(envelope);
}

bool ProjectSerializer::loadFromFile(const juce::String& filePath, ProjectState& state)
{
    juce::File file(filePath);
    if (!file.existsAsFile())
        return false;

    auto envelopeJson = file.loadFileAsString();
    if (envelopeJson.isEmpty())
        return false;

    int version = -1;
    auto stateJson = extractFromEnvelope(envelopeJson, version);

    if (version < 1 || stateJson.isEmpty())
        return false;

    // Future: handle version migration here
    // if (version < FORMAT_VERSION) { migrate(stateJson, version); }

    return state.fromJson(stateJson);
}

int ProjectSerializer::getFileVersion(const juce::String& filePath)
{
    juce::File file(filePath);
    if (!file.existsAsFile())
        return -1;

    auto envelopeJson = file.loadFileAsString();
    auto parsed = juce::JSON::parse(envelopeJson);
    if (!parsed.isObject())
        return -1;

    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return -1;

    return static_cast<int>(root->getProperty("version"));
}

} // namespace calliope
