#include <unistd.h>
#include <iostream>
#include <poll.h>
#include <sys/ioctl.h>
#include <system_error>
#include "inotify_module.h"

Inotify_module::Inotify_module(const std::string& folder):songs_folder(folder)
{
    logger = std::make_shared<Logger>("Inotify", "/home/nikita/C++/Pet_project/logs_folder/inotify_logs");
    if (!logger->whether_started())
    {
        return;
    }
    create_inotify();
    logger->make_record(std::string("Inotify started"));
}

Inotify_module::~Inotify_module()
{
    stop_watching();
    if (watching_thread.joinable())
    {
        watching_thread.join();
    }
    if (fd != -1 && wd != -1)
    {
        int ret = inotify_rm_watch(fd, wd);
        if (ret)
        {
            perror("inotify_rm_watch in destructor");
        }
    }
    if (fd != -1)
    {
        int ret = close(fd);
        if (ret)
        {
            perror("close in destructor");
        }
    }
    logger->make_record(std::string("Inotify stopped"));
}

void Inotify_module::create_inotify()
{
    fd = inotify_init1(0);
    if (fd != -1)
    {
        wd = inotify_add_watch(fd,
                               songs_folder.c_str(),
                               IN_ONLYDIR | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF);
        if (wd != -1)
        {
            if_success = true;
        } else
        {
            error_number = errno;
        }
    } else
    {
        error_number = errno;
    }
    if (if_success)
    {
        refresh_song_list();
    }
}

void Inotify_module::add_folder(std::string folder)
{
    songs_folder = folder;
    logger = std::make_shared<Logger>("Inotify", "/home/nikita/C++/Pet_project/logs_folder/inotify_logs");
    if (!logger->whether_started())
    {
        std::cerr << "Unable to start logger, folder not added" << std::endl;
        return;
    }
    create_inotify();
}

void Inotify_module::refresh_song_list() const
{
    namespace  fs = std::filesystem;
    fs::path song_folder_path{songs_folder};
    std::error_code ec;
    for (fs::directory_entry const& entry : fs::directory_iterator(song_folder_path))
    {
        if (entry.is_regular_file(ec) && (!ec) )
        {
            std::cout << (entry.path()).filename().string() << std::endl;
        }
    }
}

void Inotify_module::watching()
{
    if_success = false; //For error handling
    error_number = 0;   //For error handling
    ssize_t ret_poll;
    int timeout = 500; //Poll delay in milliseconds
    struct pollfd fds[1];
    fds[0].fd = fd; //Polling inotify descriptor
    fds[0].events = POLLIN | POLLPRI;   //Events for polling (data and priority data to read)
    while (!done)
    {
        ret_poll = poll(fds, 1, timeout);
        if (!ret_poll)
        {
            continue;
        }
        if (ret_poll == -1 && errno != EINTR)
        {
            error_number = errno;
            return;
        }
        unsigned int queue_len;
        int ret = ioctl(fd, FIONREAD, &queue_len);
        if (ret >= 0)
        {
            read_handle_event(queue_len);
        }
    }
}

void Inotify_module::read_handle_event(const int buf_size)
{
    int i = 0;
    ssize_t ret_read = 0;
    struct inotify_event* event;
    char buf[buf_size] __attribute__((aligned(__alignof__(struct inotify_event))));
    size_t event_size = sizeof(struct inotify_event);
    ret_read = read(fd, buf, buf_size);
    if (ret_read == -1 && errno != EINTR)
    {
        error_number = errno;
        return;
    }
    while (i < ret_read) //This loop is to handle all events
    {
        event = reinterpret_cast<struct inotify_event*>(&buf[i]);
        if (event->mask & IN_CREATE)
        {
            std::cout << event->name << " file created" << std::endl;
            logger->make_record(std::string(event->name) + std::string(" file created"));
        }
        if (event->mask & IN_DELETE)
        {
            std::cout << event->name << " file deleted" << std::endl;
            logger->make_record(std::string(event->name) + std::string(" file deleted"));
        }
        if (event->mask & IN_MOVED_FROM)
        {
            std::cout << event->name << " file moved from songs folder" << std::endl;
            logger->make_record(std::string(event->name) + std::string(" file moved from songs folder"));
        }
        if (event->mask & IN_MOVED_TO)
        {
            std::cout << event->name << " file moved to songs folder" << std::endl;
            logger->make_record(std::string(event->name) + std::string(" file moved to songs folder"));
        }
        if (event->mask & IN_DELETE_SELF)
        {
            std::cout << "Songs folder was deleted" << std::endl;
            logger->make_record(std::string("Songs folder was deleted"));
            songs_folder.clear();
            done = true;
        }
        if (event->mask & IN_MOVE_SELF)
        {
            std::cout << "Songs folder was moved to another location" << std::endl;
            logger->make_record(std::string("Songs folder was moved to another location"));
            songs_folder.clear();
            done = true;
        }
        i += (event_size + event->len);
    }
    unsigned int queue_len;
    int ret = ioctl(fd, FIONREAD, &queue_len);
    if (ret > 0)
    {
        read_handle_event(ret);
    }
}

bool Inotify_module::if_no_error(std::string& error) const
{
    if (error_number != 0)
    {
        error = std::string(strerror(error_number));
    } else
    {
        error = std::string("Error number is equal to zero, everything works properly");
    }
    return if_success;
}
