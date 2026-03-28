#include <catch2/catch_test_macros.hpp>
#include "calliope/command_dispatcher.h"
#include "calliope/command.h"
#include "calliope/event_types.h"
#include <memory>
#include <vector>

namespace {

// Simple test command that increments/decrements a counter
class TestCommand : public calliope::Command {
public:
    TestCommand(int& counter, int delta, bool undoable = true)
        : counter_(counter), delta_(delta), undoable_(undoable) {}

    bool perform() override {
        counter_ += delta_;
        return true;
    }

    bool undo() override {
        counter_ -= delta_;
        return true;
    }

    int getSizeInUnits() override { return 1; }

    juce::String getCommandName() const override { return "TestCommand"; }

    juce::var getEventData() const override {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("delta", delta_);
        return juce::var(obj);
    }

    bool isUndoable() const override { return undoable_; }

private:
    int& counter_;
    int delta_;
    bool undoable_;
};

// Test listener that records events
class TestListener : public calliope::CommandDispatcher::Listener {
public:
    void commandExecuted(const calliope::CommandEvent& event) override {
        executedEvents.push_back(event);
    }

    void commandUndone(const calliope::CommandEvent& event) override {
        undoneEvents.push_back(event);
    }

    std::vector<calliope::CommandEvent> executedEvents;
    std::vector<calliope::CommandEvent> undoneEvents;
};

} // anonymous namespace

TEST_CASE("CommandDispatcher dispatch executes a command", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    bool result = dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));

    REQUIRE(result == true);
    REQUIRE(counter == 5);
}

TEST_CASE("CommandDispatcher dispatch with undoable command adds to undo stack", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));

    REQUIRE(dispatcher.canUndo() == true);
}

TEST_CASE("CommandDispatcher undo reverses last command", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));
    REQUIRE(counter == 5);

    bool undone = dispatcher.undo();
    REQUIRE(undone == true);
    REQUIRE(counter == 0);
}

TEST_CASE("CommandDispatcher redo re-executes undone command", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));
    dispatcher.undo();
    REQUIRE(counter == 0);

    bool redone = dispatcher.redo();
    REQUIRE(redone == true);
    REQUIRE(counter == 5);
}

TEST_CASE("CommandDispatcher canUndo/canRedo return correct state", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    REQUIRE(dispatcher.canUndo() == false);
    REQUIRE(dispatcher.canRedo() == false);

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));
    REQUIRE(dispatcher.canUndo() == true);
    REQUIRE(dispatcher.canRedo() == false);

    dispatcher.undo();
    REQUIRE(dispatcher.canUndo() == false);
    REQUIRE(dispatcher.canRedo() == true);

    dispatcher.redo();
    REQUIRE(dispatcher.canUndo() == true);
    REQUIRE(dispatcher.canRedo() == false);
}

TEST_CASE("CommandDispatcher non-undoable command executes but skips undo stack", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    bool result = dispatcher.dispatch(std::make_unique<TestCommand>(counter, 10, false));

    REQUIRE(result == true);
    REQUIRE(counter == 10);
    REQUIRE(dispatcher.canUndo() == false);
}

TEST_CASE("CommandDispatcher Listener::commandExecuted is called after dispatch", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    TestListener listener;
    dispatcher.addListener(&listener);
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));

    REQUIRE(listener.executedEvents.size() == 1);
    REQUIRE(listener.executedEvents[0].commandName == "TestCommand");
    REQUIRE(listener.executedEvents[0].type == calliope::EventType::CommandExecuted);
    REQUIRE(listener.executedEvents[0].isUndo == false);

    dispatcher.removeListener(&listener);
}

TEST_CASE("CommandDispatcher Listener::commandUndone is called after undo", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    TestListener listener;
    dispatcher.addListener(&listener);
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));
    dispatcher.undo();

    REQUIRE(listener.undoneEvents.size() == 1);
    REQUIRE(listener.undoneEvents[0].commandName == "TestCommand");
    REQUIRE(listener.undoneEvents[0].type == calliope::EventType::CommandUndone);
    REQUIRE(listener.undoneEvents[0].isUndo == true);

    dispatcher.removeListener(&listener);
}

TEST_CASE("CommandDispatcher multiple listeners all receive events", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    TestListener listener1;
    TestListener listener2;
    dispatcher.addListener(&listener1);
    dispatcher.addListener(&listener2);
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));

    REQUIRE(listener1.executedEvents.size() == 1);
    REQUIRE(listener2.executedEvents.size() == 1);

    dispatcher.removeListener(&listener1);
    dispatcher.removeListener(&listener2);
}

TEST_CASE("CommandDispatcher undo stack holds at least 100 operations", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    // Insert 150 commands
    for (int i = 0; i < 150; ++i) {
        dispatcher.dispatch(std::make_unique<TestCommand>(counter, 1));
    }
    REQUIRE(counter == 150);

    // Undo 100 times successfully
    int undoCount = 0;
    for (int i = 0; i < 100; ++i) {
        if (dispatcher.undo())
            ++undoCount;
    }

    REQUIRE(undoCount == 100);
    REQUIRE(counter == 50);
}

TEST_CASE("CommandDispatcher getUndoDescription/getRedoDescription return command names", "[dispatcher]") {
    calliope::CommandDispatcher dispatcher;
    int counter = 0;

    dispatcher.dispatch(std::make_unique<TestCommand>(counter, 5));

    REQUIRE(dispatcher.getUndoDescription() == "TestCommand");

    dispatcher.undo();

    REQUIRE(dispatcher.getRedoDescription() == "TestCommand");
}
