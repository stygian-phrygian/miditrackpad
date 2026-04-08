#include "portmidi.h"
#include "portmidi_raii.h"
#include <iostream>
#include <stdexcept>

PortMidi::PortMidi()
{
    PmError status{};
    status = Pm_Initialize();
    if (status != pmNoError)
    {
        throw std::runtime_error(Pm_GetErrorText(status));
    }
}

PortMidi::~PortMidi()
{
    Pm_Terminate();
}

const int PortMidi::get_device_count() const
{
    return Pm_CountDevices();
}

const std::optional<PortMidi::DeviceInfo> PortMidi::get_device_info(const int device_id) const
{
    // NB. PortMidi owns the returned device info ptr, DO NOT clean it up.
    const PmDeviceInfo* pm_device_info = Pm_GetDeviceInfo(device_id);
    if (nullptr == pm_device_info)
    {
        return std::nullopt;
    }

    return PortMidi::DeviceInfo{
        pm_device_info->name,
            pm_device_info->input  != 0,
            pm_device_info->output != 0,
            pm_device_info->opened != 0
    };
}

PortMidi::Stream PortMidi::open_input_stream(const int device_id, const int buffer_size)
{
    bool is_input = true;
    return Stream(device_id, is_input, buffer_size);
}

PortMidi::Stream PortMidi::open_output_stream(const int device_id)
{
    bool is_input = false;
    return Stream(device_id, is_input);
}

std::ostream& operator<<(std::ostream& os, const PortMidi::DeviceInfo& device_info)
{
    os  << "{"
        << "name: "         << device_info.name         << ", "
        << "is_input: "     << device_info.is_input     << ", "
        << "is_output: "    << device_info.is_output    << ", "
        << "is_opened: "    << device_info.is_opened
        << "}";
    return os;
}

PortMidi::Stream::Stream(const int device_id, const bool is_input, const int buffer_size)
{
    // open stream
    PortMidiStream* stream;
    PmError status;
    if (is_input)
    {
        status = Pm_OpenInput(
                &stream,
                device_id,
                nullptr,        // additional driver info
                buffer_size,    // midi buffer size
                nullptr,        // time function which returns elapsed milliseconds (null for PortTime default)
                nullptr);       // pointer to pass to the time function (above)

    }
    else
    {
        status = Pm_OpenOutput(
                &stream,
                device_id,
                nullptr,        // additional driver info
                0,              // midi buffer size (0 for default)
                nullptr,        // time function which returns elapsed milliseconds (null for PortTime default)
                nullptr,        // pointer to pass to the time function (above)
                0);             // message latency (0 for no delay)
    }

    if (status != pmNoError)
    {
        throw std::runtime_error(Pm_GetErrorText(status));
    }

    m_stream = stream;
}

PortMidi::Stream::~Stream()
{
    if (m_stream) { Pm_Close(m_stream); }
}

bool PortMidi::Stream::write_short(Message msg)
{
    const auto status = Pm_WriteShort(m_stream, 0 /* now */, msg);
    if (status != pmNoError)
    {
        std::cout << "MIDI error: " << Pm_GetErrorText(status) << "\n";
    }
    return status == pmNoError;
}

