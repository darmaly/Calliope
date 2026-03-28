include_directories(${CMAKE_JS_INC})

add_definitions(-DNAPI_VERSION=9)

add_library(calliope_addon SHARED
    src/addon.cpp
    src/bridge.cpp
    ${CMAKE_JS_SRC}
)

set_target_properties(calliope_addon PROPERTIES
    PREFIX ""
    SUFFIX ".node"
)

target_link_libraries(calliope_addon
    PRIVATE
        calliope_engine
        ${CMAKE_JS_LIB}
)

execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(calliope_addon PRIVATE ${NODE_ADDON_API_DIR})
