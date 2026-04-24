add_executable(checkpp-release-tool
    release_tool.cpp
)

set_target_properties(checkpp-release-tool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
