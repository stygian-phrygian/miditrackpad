cmake_minimum_required(VERSION 3.15)

project(MidiTrackpad)

# Add find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Add dependencies
find_package(EvDev REQUIRED)
find_package(PortMidi REQUIRED)

# Add library target
add_subdirectory(src)

# Add executable target
add_subdirectory(apps)

