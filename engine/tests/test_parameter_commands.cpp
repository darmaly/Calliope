#include <catch2/catch_test_macros.hpp>
#include "calliope/commands/parameter_commands.h"
#include "calliope/parameter_registry.h"

using namespace calliope;

TEST_CASE("SetParameterCommand with valid ID executes getter/setter through registry", "[parameter_commands]") {
    ParameterRegistry registry;
    double value = 120.0;

    registry.registerParameter("test.param", {
        [&]() -> juce::var { return value; },
        [&](const juce::var& v) { value = static_cast<double>(v); },
        "double", 0.0, 300.0
    });

    SetParameterCommand cmd(registry, "test.param", juce::var(200.0));
    bool result = cmd.perform();

    REQUIRE(result == true);
    REQUIRE(value == 200.0);
}

TEST_CASE("SetParameterCommand::undo() restores previous value via getter/setter", "[parameter_commands]") {
    ParameterRegistry registry;
    double value = 120.0;

    registry.registerParameter("test.param", {
        [&]() -> juce::var { return value; },
        [&](const juce::var& v) { value = static_cast<double>(v); },
        "double", 0.0, 300.0
    });

    SetParameterCommand cmd(registry, "test.param", juce::var(200.0));
    cmd.perform();
    REQUIRE(value == 200.0);

    cmd.undo();
    REQUIRE(value == 120.0);
}

TEST_CASE("SetParameterCommand with unknown ID returns false from perform()", "[parameter_commands]") {
    ParameterRegistry registry;

    SetParameterCommand cmd(registry, "nonexistent.param", juce::var(42));
    bool result = cmd.perform();

    REQUIRE(result == false);
}

TEST_CASE("SetParameterCommand::getCommandName() includes the parameter ID", "[parameter_commands]") {
    ParameterRegistry registry;
    double value = 0.0;

    registry.registerParameter("my.param", {
        [&]() -> juce::var { return value; },
        [&](const juce::var& v) { value = static_cast<double>(v); },
        "double", 0.0, 100.0
    });

    SetParameterCommand cmd(registry, "my.param", juce::var(50.0));

    REQUIRE(cmd.getCommandName().contains("my.param"));
}

TEST_CASE("SetParameterCommand::getEventData() contains parameter ID and new value", "[parameter_commands]") {
    ParameterRegistry registry;
    double value = 0.0;

    registry.registerParameter("test.param", {
        [&]() -> juce::var { return value; },
        [&](const juce::var& v) { value = static_cast<double>(v); },
        "double", 0.0, 100.0
    });

    SetParameterCommand cmd(registry, "test.param", juce::var(50.0));

    auto data = cmd.getEventData();
    REQUIRE(data.hasProperty("parameterId"));
    REQUIRE(data["parameterId"].toString() == "test.param");
    REQUIRE(data.hasProperty("value"));
    REQUIRE(static_cast<double>(data["value"]) == 50.0);
}
