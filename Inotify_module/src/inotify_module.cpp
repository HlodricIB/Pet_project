#include <unistd.h>
#include <iostream>
#include "inotify_module.h"

Inotify_module::Inotify_module(const char* folder)
{
    fd = inotify_init1(0);
    if (fd != -1)
    {
        wd = inotify_add_watch(fd,
                               folder,
                               IN_ONLYDIR | IN_CREATE | IN_DELETE | IN_MOVE | IN_DELETE_SELF | IN_MOVE_SELF);
        if (wd != -1)
        {
            initialized = true;
            start_watching();
        } else
        {
            error_number = errno;
        }
    } else
    {
        error_number = errno;
    }
}

void Inotify_module::start_watching()
{
    const size_t multiplier = 10;
    const size_t avg_fname_size = 10;
    const size_t event_size = sizeof(struct inotify_event);
    constexpr size_t buf_size = multiplier * (event_size + avg_fname_size);
    char buf[buf_size] __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t ret, i = 0;
    size_t len_buf = buf_size;
    char* pos = &buf[0];
    struct inotify_event* event;
    while (!done)
    {
        while (len_buf != 0 && (ret = read(fd, pos, len_buf)) != 0) //This loop is to read all events from fd
        {
            if (ret == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                } else
                {
                    break;
                }
            }
            len_buf -= ret;
            pos += ret;
        }
        while (i < ret)
        {
            event = reinterpret_cast<struct inotify_event*>(&buf[i]);
            if (static_cast<bool>(event->len))
            {
                std::cout << event->name << std::endl;
            }
            i += event_size + event->len;
        }
    }

}

bool Inotify_module::is_initialized(std::string& error) const
{
    if (error_number != 0)
    {
        error = std::string(strerror(error_number));
    } else
    {
        error = std::string("Error number is equal to zero, what shouldn't happen");
    }
    return initialized;
}
