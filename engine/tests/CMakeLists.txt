include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(calliope_engine_tests
    test_main.cpp
    test_transport.cpp
    test_lock_free_queue.cpp
)

target_link_libraries(calliope_engine_tests
    PRIVATE
        calliope_engine
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(calliope_engine_tests)
