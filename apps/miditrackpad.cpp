#include <algorithm>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include "evdev_raii.h"
#include "portmidi_raii.h"

// globals
const char        MIDI_PITCH_BEND               = 0xE0;
const char        MIDI_CONTROL_CHANGE           = 0xB0; // mod wheel (CC1)
const char        MIDI_CHANNEL_AFTERTOUCH       = 0xD0;
const std::string evdev_device_directory        = "/dev/input/";
std::atomic<bool> is_running                    = true;

void signal_handler(int signal)
{
    switch (signal)
    {
        case SIGINT:
            is_running = false;
            break;
    }
}

int get_event_id(const std::string& event_path)
{
    size_t idx = event_path.rfind("event");
    if (std::string::npos == idx)
    {
        throw std::runtime_error("Could not parse id from event path");
    }
    return stoi(event_path.substr(idx + 5));
}

void print_input_info()
{
    // collate all paths in evdev device directory with file names prefixed by "event"
    std::vector<std::filesystem::path> event_paths{};
    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(evdev_device_directory))
        {
            const std::string filename = entry.path().filename().string();
            if (filename.find("event") == 0) { event_paths.push_back(entry.path()); }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Cannot access " << evdev_device_directory << ": " << e.what() << "\n";
        return;
    }
    std::sort(
            std::begin(event_paths),
            std::end(event_paths),
            [](const std::string& a, const std::string& b)
            {
                return get_event_id(a) < get_event_id(b);
            });

    // print device info
    std::cout << "Inputs:\n";
    for (auto p : event_paths)
    {
        std::cout << "\t" << p << " " << EvDev(p) << std::endl;
    }
}

void print_output_info()
{
    auto midi = PortMidi{};
    std::cout << "Outputs (MIDI):\n";
    for (int i = 0; i < midi.get_device_count(); ++i)
    {
        auto d = midi.get_device_info(i);
        if (d and d->is_output)
        {
            std::cout << "\tid: " << i << ", name: " << d->name << "\n";
        }
    }
}

// debugging
void print_event(input_event e)
{
    std::cout <<
        libevdev_event_type_get_name(e.type) << ": " <<
        libevdev_event_code_get_name(e.type, e.code) << " " <<
        "{" << e.value << "}" << std::endl;
}

int main(int argc, char** argv)
{
    // register a handler for SIGINT (ctrl-c)
    std::signal(SIGINT, signal_handler);

    // parse command line options
    int                         opt{};
    int                         timeout_ms{16}; // ~ 60Hz
    std::optional<std::string>  trackpad_device_path;
    std::optional<int>          midi_device_id;
    unsigned char               MIDI_CHANNEL = 0;
    while ((opt = getopt(argc, argv, "lt:i:o:c:")) != -1)
    {
        switch (opt)
        {
            // list (trackpad) inputs and (midi) outputs
            case 'l':
            {
                print_input_info();
                print_output_info();
                return 0;
            }

            case 't':
            {
                timeout_ms = std::stoi(std::string(optarg));
                std::cout << std::string{optarg} << " and " << timeout_ms << "\n";
                break;
            }

            // trackpad device id or path
            case 'i':
            {
                // parse both cases, when optarg is:
                //     1. an integer device id, eg. "12"
                //     2. a path, eg. "/dev/input/event12"
                std::stringstream ss{optarg};
                int device_id = -1;
                if (ss >> device_id and ss.eof())
                {
                    // optarg is a valid integer string
                    // assemble the device path
                    trackpad_device_path = evdev_device_directory + "event" + std::to_string(device_id);
                }
                else
                {
                    // optarg is (hopefully) a directory path string
                    trackpad_device_path = optarg;
                }

                std::cout << "Parsed trackpad device path: " << *trackpad_device_path << "\n";
                break;
            }

            // midi device id
            case 'o':
            {
                midi_device_id = std::stoi(std::string{optarg});
                std::cout << "Parsed midi device id: " << *midi_device_id << "\n";
                break;
            }

            // midi channel [0-15]
            case 'c':
            {
                MIDI_CHANNEL = std::max(0, std::min(15, std::stoi(std::string{optarg})));
                std::cout << "Parsed midi channel: " << MIDI_CHANNEL << "\n";
                break;
            }
        }
    }

    if (not midi_device_id or not trackpad_device_path)
    {
        std::cout << "Could not open both devices.\n";
        return -1;
    }

    // open evdev device
    EvDev evdev_device{*trackpad_device_path};
    evdev_device.grab();
    // assert it supports absolute x y z coordinate events
    const auto x_axis = evdev_device.get_abs_info(ABS_X);
    const auto y_axis = evdev_device.get_abs_info(ABS_Y);
    const auto z_axis = evdev_device.get_abs_info(ABS_PRESSURE);
    if (not x_axis or not y_axis or not z_axis)
    {
        std::cout << "Device: {" + evdev_device.get_name() + "} does not support absolute X Y or Z (pressure) events.\n";
        return -1;
    }
    // for normalization, acquire min/max of each axis
    const int x_center  = (x_axis->maximum + x_axis->minimum) / 2;
    const int y_center  = (y_axis->maximum + y_axis->minimum) / 2;
    const int x_min     = x_axis->minimum;
    const int x_max     = x_axis->maximum;
    const int y_min     = y_axis->minimum;
    const int y_max     = y_axis->maximum;
    const int z_min     = z_axis->minimum;
    const int z_max     = z_axis->maximum;

    // open portmidi device
    PortMidi midi{};
    PortMidi::Stream midi_device{midi.open_output_stream(*midi_device_id)};

    // read events
    std::vector<input_event> events{};
    events.reserve(64);
    while (is_running)
    {
        events.clear();
        evdev_device.read(events, timeout_ms);
        for (auto e : events)
        {
            // print_event(e); // DEBUG

            // absolute coordinates
            if(e.type == EV_ABS)
            {
                switch (e.code)
                {
                    // translate normalized x coordinate to midi pitchwheel
                    case ABS_X:
                    {
                        const auto x        = e.value;
                        const auto x_norm   = double(x - x_center) / ((x_max - x_min) / 2);
                        const auto pitch    = static_cast<int32_t>(8192.0 + x_norm * 8191.0);
                        midi_device.write_short(
                                PortMidi::Message(
                                    // status byte
                                    MIDI_PITCH_BEND | MIDI_CHANNEL,
                                    // data1 byte (lower 7 bits)
                                    pitch & 0x7F,
                                    // data2 byte (upper 7 bits)
                                    (pitch >> 7) & 0x7F));
                        // std::cout << "X: " << x << " X_NORM: " << x_norm << " PB: " << pitch << std::endl; // DEBUG
                        break;
                    }

                    // translate normalized y coordinate to midi modwheel
                    case ABS_Y:
                    {
                        const auto y        = e.value;
                        // flip polarity, because in the apple trackpad y-axis: ↑ is negative and ↓ is positive
                        const auto y_norm   = -(double(y - y_center) / ((y_max - y_min) / 2));
                        const char cc_num   = 0x01;
                        const char cc_val   = (y_norm + 1.0) * 0.5 * 127;
                        midi_device.write_short(
                                PortMidi::Message(
                                    // status byte
                                    MIDI_CONTROL_CHANGE | MIDI_CHANNEL,
                                    // data1 byte
                                    cc_num,
                                    // data2 byte
                                    cc_val));
                        // std::cout << "Y: " << y << " Y_NORM: " << y_norm << " MOD: " << static_cast<int>(cc_val) << std::endl; // DEBUG
                        break;
                    }

                    // translate normalized z coordinate (pressure) to midi channel aftertouch
                    case ABS_PRESSURE:
                    {
                        const auto z        = e.value;
                        const auto z_norm   = double(z) / z_max;
                        const char chan_at  = z_norm * 127;
                        midi_device.write_short(
                                PortMidi::Message(
                                    // status byte
                                    MIDI_CHANNEL_AFTERTOUCH | MIDI_CHANNEL,
                                    // data1 byte
                                    chan_at,
                                    // data2 byte (unused)
                                    0));
                        // std::cout << "Z: " << z << " Z_NORM: " << z_norm << " ChanAt: " << static_cast<int>(chan_at) << std::endl; // DEBUG
                        break;
                    }
                }
            }
            // touch release
            else if (e.type == EV_KEY and e.code == BTN_TOUCH and e.value == 0)
            {
                // reset pitch wheel (x-axis) to center
                const int32_t pitch = 8192;
                midi_device.write_short(
                        PortMidi::Message(
                            // status byte
                            MIDI_PITCH_BEND | MIDI_CHANNEL,
                            // data1 byte (lower 7 bits)
                            pitch & 0x7F,
                            // data2 byte (upper 7 bits)
                            (pitch >> 7) & 0x7F));

                // reset channel aftertouch (z-axis) to zero
                // note, removing pressure necessarily resets this axis, therefore
                // this is likely unnecessary
                const char chan_at = 0;
                midi_device.write_short(
                        PortMidi::Message(
                            // status byte
                            MIDI_CHANNEL_AFTERTOUCH | MIDI_CHANNEL,
                            // data1 byte
                            chan_at,
                            // data2 byte (unused)
                            0));
            }
        }
    }

    return 0;
}

