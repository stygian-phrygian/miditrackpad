# evdev RAII wrapper

add_library(
    evdev_raii
    evdev_raii.cpp)

target_include_directories(
    evdev_raii
    PUBLIC ../include)

target_link_libraries(
    evdev_raii
    PUBLIC evdev::evdev)

target_compile_features(
    evdev_raii
    PRIVATE cxx_std_20)

# portmidi RAII wrapper

add_library(
    portmidi_raii
    portmidi_raii.cpp)

target_include_directories(
    portmidi_raii
    PUBLIC ../include)

target_link_libraries(
    portmidi_raii
    PRIVATE portmidi::portmidi)

target_compile_features(
    portmidi_raii
    PRIVATE cxx_std_20)

