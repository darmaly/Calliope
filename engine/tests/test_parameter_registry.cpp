#include <catch2/catch_test_macros.hpp>
#include "calliope/parameter_registry.h"
#include <algorithm>

TEST_CASE("ParameterRegistry registerParameter stores a parameter", "[parameter_registry]") {
    calliope::ParameterRegistry registry;
    float value = 0.5f;

    registry.registerParameter("test.param", {
        [&value]() -> juce::var { return value; },
        [&value](const juce::var& v) { value = static_cast<float>(v); },
        "float", 0.0f, 1.0f
    });

    auto* param = registry.getParameter("test.param");
    REQUIRE(param != nullptr);
}

TEST_CASE("ParameterRegistry getParameter returns nullptr for unknown ID", "[parameter_registry]") {
    calliope::ParameterRegistry registry;

    auto* param = registry.getParameter("nonexistent.param");
    REQUIRE(param == nullptr);
}

TEST_CASE("ParameterRegistry getAllParameterIds returns all registered IDs", "[parameter_registry]") {
    calliope::ParameterRegistry registry;
    float a = 0.0f, b = 0.0f;

    registry.registerParameter("param.a", {
        [&a]() -> juce::var { return a; },
        [&a](const juce::var& v) { a = static_cast<float>(v); },
        "float", 0.0f, 1.0f
    });

    registry.registerParameter("param.b", {
        [&b]() -> juce::var { return b; },
        [&b](const juce::var& v) { b = static_cast<float>(v); },
        "float", 0.0f, 1.0f
    });

    auto ids = registry.getAllParameterIds();
    REQUIRE(ids.size() == 2);

    // std::map is ordered, so param.a comes before param.b
    REQUIRE(std::find(ids.begin(), ids.end(), juce::String("param.a")) != ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), juce::String("param.b")) != ids.end());
}

TEST_CASE("ParameterRegistry getter/setter work through the registry", "[parameter_registry]") {
    calliope::ParameterRegistry registry;
    float value = 0.5f;

    registry.registerParameter("volume", {
        [&value]() -> juce::var { return value; },
        [&value](const juce::var& v) { value = static_cast<float>(v); },
        "float", 0.0f, 1.0f
    });

    auto* param = registry.getParameter("volume");
    REQUIRE(param != nullptr);

    // Read initial value
    REQUIRE(static_cast<float>(param->getter()) == 0.5f);

    // Set new value via setter
    param->setter(0.8f);
    REQUIRE(value == 0.8f);

    // Verify getter reflects the change
    REQUIRE(static_cast<float>(param->getter()) == 0.8f);
}

TEST_CASE("ParameterRegistry parameter metadata is stored and retrievable", "[parameter_registry]") {
    calliope::ParameterRegistry registry;
    int value = 50;

    registry.registerParameter("tempo", {
        [&value]() -> juce::var { return value; },
        [&value](const juce::var& v) { value = static_cast<int>(v); },
        "int", 20, 300
    });

    auto* param = registry.getParameter("tempo");
    REQUIRE(param != nullptr);
    REQUIRE(param->type == "int");
    REQUIRE(static_cast<int>(param->minValue) == 20);
    REQUIRE(static_cast<int>(param->maxValue) == 300);
}
