#include <unistd.h>
#include <iostream>
#include <poll.h>
#include <sys/ioctl.h>
#include <system_error>
#include "inotify_module.h"

Inotify_module::Inotify_module(const std::string& songs_folder_, const std::string& logs_folder):songs_folder(songs_folder_)
{
    if (!create_dir_if_not_exist())
    {
        return;
    }
    logger = std::make_unique<Logger>("Inotify", logs_folder.c_str());
    if (!logger->whether_started())
    {
        return;
    }
    create_inotify();
}

Inotify_module::Inotify_module(const std::string& songs_folder_, std::unique_ptr<Logger> logger_): songs_folder(songs_folder_), logger(std::move(logger_))
{
    if (!logger->whether_started())
    {
        return;
    }
    create_inotify();
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
    if (if_success && logger)
    {
        logger->make_record(std::string("Inotify started"));
        refresh_song_list();
        logger->make_record("Song list refreshed");
    } else
    {
        if (if_success)
        {
            refresh_song_list();
        } else
        {
            if (logger)
            {
                logger->make_record("Inotify failed to start");
            }
        }
    }
}

bool Inotify_module::create_dir_if_not_exist()
{
    namespace fs = std::filesystem;
    fs::path songs_folder_path{songs_folder};
    if (fs::exists(songs_folder_path))
    {
        return true;
    }
    std::cout << "Specified songs directory doesn't exist, create ";
    char c;
    std::error_code e_c;
    while(true)
    {
        std::cout << "[y/n]?\n";
        if (!(std::cin >> c))
        {
            std::cin.clear();
            continue;
        }
        std::cin.get();
        switch (c)
        {
        case 'y' :
            if (!std::filesystem::create_directories(songs_folder_path, e_c))
            {
                std::cerr << "Unable to create specified directory for songs files: " << e_c.message() << std::endl;
                return false;
            }
            return true;
            break;
        case 'n' :
            return false;
            break;
        default:
            if (std::cin.rdstate() == std::ios_base::badbit)
            {
                return false;
            } else
            {
                continue;
            }
        }
    }
}

void Inotify_module::set_folder(std::string folder)
{
    if (!create_dir_if_not_exist())
    {
        return ;
    }
    songs_folder = folder;
    logger = std::make_unique<Logger>("Inotify", folder.c_str());
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
    std::vector<std::string> song_list(1, "refresh");
    for (fs::directory_entry const& entry : fs::directory_iterator(song_folder_path))
    {
        if (entry.is_regular_file(ec) && (!ec) )
        {
            song_list.push_back((entry.path()).filename().string());
        }
    }
    handler->handle(song_list);
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
            if (logger)
            {
                logger->make_record(std::string(event->name) + std::string(" file created"));
            }

            handler->handle({"add", std::string(event->name)});
        }
        if (event->mask & IN_DELETE)
        {
            if (logger)
            {
                logger->make_record(std::string(event->name) + std::string(" file deleted"));
            }
            handler->handle({"delete", std::string(event->name)});
        }
        if (event->mask & IN_MOVED_FROM)
        {
            if (logger)
            {
                logger->make_record(std::string(event->name) + std::string(" file moved from songs folder"));
            }
            handler->handle({"delete", std::string(event->name)});
        }
        if (event->mask & IN_MOVED_TO)
        {
            if (logger)
            {
                logger->make_record(std::string(event->name) + std::string(" file moved to songs folder"));
            }
            handler->handle({"add", std::string(event->name)});
        }
        if (event->mask & IN_DELETE_SELF)
        {
            if (logger)
            {
                logger->make_record(std::string("Songs folder was deleted"));
            }
            handler->handle({"dir_del"});
            songs_folder.clear();
            done = true;
        }
        if (event->mask & IN_MOVE_SELF)
        {
            if (logger)
            {
                logger->make_record(std::string("Songs folder was moved to another location"));
            }
            handler->handle({"dir_del"});
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
