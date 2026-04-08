# Add search locations
# Append the GNU install directories to CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)

# Set the header location in a cache variable
find_path(
    PORTMIDI_INCLUDE_DIR
    portmidi.h
    HINTS ${CMAKE_INSTALL_INCLUDEDIR})

# Set the library location in a cache variable
find_library(
    PORTMIDI_LIBRARY
    portmidi)

# Force our find module to behave in a standard way
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    PortMidi
    DEFAULT_MSG
    PORTMIDI_LIBRARY
    PORTMIDI_INCLUDE_DIR
)

# Hide cache variables from displaying in the GUI
mark_as_advanced(PORTMIDI_LIBRARY PORTMIDI_INCLUDE_DIR)

# Add imported library target
if(PORTMIDI_FOUND AND NOT TARGET portmidi::portmidi)
    add_library(portmidi::portmidi SHARED IMPORTED)
    set_target_properties(
        portmidi::portmidi
        PROPERTIES

        INTERFACE_INCLUDE_DIRECTORIES
        "${PORTMIDI_INCLUDE_DIR}"

        IMPORTED_LOCATION
        ${PORTMIDI_LIBRARY})
endif()
