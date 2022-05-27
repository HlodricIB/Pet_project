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
