# Add search locations
# Append the GNU install directories to CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)

# Set the header location in a cache variable
find_path(
    EVDEV_INCLUDE_DIR
    NAMES libevdev/libevdev.h
    PATHS /usr/include /usr/local/include
    # This looks for 'libevdev/libevdev.h' inside '/usr/include/libevdev-1.0/'
    # So the include path becomes '/usr/include/libevdev-1.0'
    PATH_SUFFIXES libevdev-1.0)

# Set the library location in a cache variable
find_library(
    EVDEV_LIBRARY
    NAMES evdev)

# Force our find module to behave in a standard way
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    EvDev
    DEFAULT_MSG
    EVDEV_LIBRARY
    EVDEV_INCLUDE_DIR
)

# Hide cache variables from displaying in the GUI
mark_as_advanced(EVDEV_LIBRARY EVDEV_INCLUDE_DIR)

# Add imported library target
if(EVDEV_FOUND AND NOT TARGET evdev::evdev)
    add_library(evdev::evdev SHARED IMPORTED)
    set_target_properties(
        evdev::evdev
        PROPERTIES

        INTERFACE_INCLUDE_DIRECTORIES
        "${EVDEV_INCLUDE_DIR}"

        IMPORTED_LOCATION
        ${EVDEV_LIBRARY})
endif()
