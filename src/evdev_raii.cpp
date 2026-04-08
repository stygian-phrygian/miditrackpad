#include "libevdev/libevdev.h"
#include "evdev_raii.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstring>
#include <poll.h>
// for POSIX
#include <fcntl.h>      // For open() and flags like O_RDWR
#include <sys/stat.h>   // For file permissions, e.g., S_IRUSR
#include <unistd.h>     // For close()


EvDev::EvDev(const std::string& device_path)
{
    // open fd (non-blocking)
    const int fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        throw std::runtime_error("failed open(): " + device_path);
    }

    // open evdev device
    libevdev* device = nullptr;
    const int status = libevdev_new_from_fd(fd, &device);
    if (status != 0)
    {
        close(fd);
        throw std::runtime_error("failed libevdev_new_from_fd(): " + device_path);
    }

    // assign members
    m_fd = fd;
    m_device = device;
}

EvDev::EvDev(const int device_id) :
    EvDev("/dev/input/event" + std::to_string(device_id))
{ }

EvDev::~EvDev()
{
    if (nullptr != m_device)
    {
        ungrab();
        libevdev_free(m_device);
        m_device = nullptr;
    }

    if (-1 != m_fd)
    {
        close(m_fd);
        m_fd = -1;
    }
}

std::string EvDev::get_name() const
{
    return libevdev_get_name(m_device);
}

void EvDev::grab()
{
    libevdev_grab(m_device, LIBEVDEV_GRAB);
}

void EvDev::ungrab()
{
    libevdev_grab(m_device, LIBEVDEV_UNGRAB);
}

bool EvDev::has_event_type(unsigned int type) const
{
    return libevdev_has_event_type(m_device, type);
}

bool EvDev::has_event_code(unsigned int type, unsigned int code) const
{
    return libevdev_has_event_code(m_device, type, code);
}

std::optional<const input_absinfo> EvDev::get_abs_info(unsigned int code) const
{
    const input_absinfo* info = libevdev_get_abs_info(m_device, code);
    if (nullptr == info) { return std::nullopt; }
    return *info;
}

void EvDev::read(std::vector<input_event>& events, int timeout)
{
    // poll fd for (input) events with (ms) timeout
    pollfd pfd{m_fd, POLLIN, 0};
    int status = poll(&pfd, 1, timeout);

    // has polling errors
    if (status == -1)
    {
        int saved_errno = errno;
        // ignore EINTR, this is an interrupt, probably SIGINT in our case
        if (saved_errno == EINTR) { return; }
        throw std::system_error(saved_errno, std::generic_category(), "poll() failed");
    }

    // has no events
    if (status == 0)
    {
        return; // timeout
    }

    // has events AND device errors
    if (pfd.revents & (POLLERR /* device state error */ | POLLHUP /* device connect lost */ | POLLNVAL /* fd already closed */))
    {
        throw std::runtime_error("failed read(): input device has errors");
    }

    // drain *all* events when we wake up
    while (true)
    {
        // read an event
        input_event e{};
        status = libevdev_next_event(
                m_device,
                LIBEVDEV_READ_FLAG_NORMAL,
                &e);

        // no more events right now
        if (status == -EAGAIN)
        {
            break;
        }

        // normal event read
        if (LIBEVDEV_READ_STATUS_SUCCESS)
        {
            events.push_back(e);
        }
        // sync queue drain
        // if events were not read quickly enough, kernel injects them with a SYNC
        else if (LIBEVDEV_READ_STATUS_SYNC or status == SYN_DROPPED)
        {
            do
            {
                events.push_back(e);
                status = libevdev_next_event(
                        m_device,
                        LIBEVDEV_READ_FLAG_SYNC,
                        &e);
            }
            while (status == LIBEVDEV_READ_STATUS_SYNC);
        }
        // unexpected error
        else if (status < 0)
        {
            std::cerr << "failed to read next event: " << std::strerror(-status) << std::endl;
            break;
        }
    }

}

std::ostream& operator<<(std::ostream& os, const EvDev& device)
{
    os << "{" << device.get_name() << "}";
    return os;
}

