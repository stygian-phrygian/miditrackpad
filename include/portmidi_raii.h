#ifndef PORTMIDI_RAII_H
#define PORTMIDI_RAII_H

#include "portmidi.h"
#include <iostream>
#include <string>
#include <optional>

// wraps portmidi in RAII
class PortMidi
{
public:
    PortMidi();
    ~PortMidi();

    // returns count of available devices
    const int get_device_count() const;

    // wraps a PmDeviceInfo* for value semantics
    // PmDeviceInfo struct (relevant members):
    //     char *name;     // string name
    //     int input;      // bool (is input available?)
    //     int output;     // bool (if outout available?)
    //     int opened;     // bool (is opened?)
    struct DeviceInfo
    {
    public:
        std::string name;
        bool is_input;
        bool is_output;
        bool is_opened;

        friend std::ostream& operator<<(std::ostream& os, const DeviceInfo& device_info);
    };

    // returns device info (if available)
    // device id is always a whole number {0, ..., N}
    const std::optional<DeviceInfo> get_device_info(const int device_id) const;

    // wrapper for a midi message
    class Message
    {
    public:
        Message(char status, char data1, char data2) :
            m_bytes{Pm_Message(status, data1, data2)}
        { }

        operator PmMessage() const
        {
            return m_bytes;
        }

    private:
        uint32_t m_bytes;
    };

    // wraps a PortMidiStream* in RAII
    class Stream
    {
    public:
        Stream(const int device_id, const bool is_input, const int buffer_size=128);
        Stream(const Stream&) = delete;            // no copy ctor
        Stream& operator=(const Stream&) = delete; // no copy assignment operator
        ~Stream();

        // TODO:
        // * read
        // * poll
        // * write

        // writes a single midi message
        bool write_short(Message msg);

    private:
        PortMidiStream* m_stream;
    };

    // returns an *input* stream
    Stream open_input_stream(const int device_id, const int buffer_size=128);

    // returns an *output* stream
    Stream open_output_stream(const int device_id);

};

#endif



