cmake_minimum_required(VERSION 3.10)
project(ChessEngine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SFML
find_package(SFML 2 COMPONENTS graphics window system REQUIRED)

# Add source files
set(SOURCES
    src/main.cpp
    src/Board.cpp
    src/Engine.cpp
    src/GUI.cpp
)

# Create executable
add_executable(ChessEngine ${SOURCES})

# Copy assets directory to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Link SFML libraries
target_link_libraries(ChessEngine sfml-graphics sfml-window sfml-system)
