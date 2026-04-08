add_executable(
    miditrackpad
    miditrackpad.cpp)

target_link_libraries(
    miditrackpad
    PRIVATE evdev_raii)

target_link_libraries(
    miditrackpad
    PRIVATE portmidi_raii)

target_compile_features(
    miditrackpad
    PRIVATE cxx_std_20)

