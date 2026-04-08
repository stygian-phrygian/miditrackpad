#ifndef EVDEV_RAII_H
#define EVDEV_RAII_H

#include "libevdev/libevdev.h"
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <optional>

// wraps evdev devices — which support absolute XYZ coordinates —  in RAII
class EvDev
{
public:
    EvDev(const std::string& device_path);
    EvDev(const int device_id);
    EvDev(const EvDev&) = delete;            // no copy ctor
    EvDev& operator=(const EvDev&) = delete; // no copy assignment operator
    ~EvDev();

    std::string get_name() const;

    void grab();
    void ungrab();

    // get device info, event types, codes, and resolutions
    bool has_event_type(unsigned int type) const;
    bool has_event_code(unsigned int type, unsigned int code) const;
    std::optional<const input_absinfo> get_abs_info(unsigned int code) const;

    // read event(s) into output vector
    // either a single event or multiple sync drained events
    // if timeout < 0 then block
    // else then wait at most timeout ms for events
    void read(std::vector<input_event>& events, int timeout = -1 /* ms */);

    friend std::ostream& operator<<(std::ostream& os, const EvDev& device);

private:
    int         m_fd = -1;
    libevdev*   m_device = nullptr;
};

#endif
