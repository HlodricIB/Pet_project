#ifndef INOTIFY_MODULE_H
#define INOTIFY_MODULE_H

#include <sys/inotify.h>
#include <string>
#include <string.h>
#include <errno.h>
#include <atomic>
#include <thread>
#include <filesystem>
#include <memory>
#include "logger.h"

class Inotify_module
{
private:
    std::string songs_folder;
    std::shared_ptr<Logger> logger{nullptr};
    bool if_success{false};    //Flag that shows if function works successfully
    int error_number{0};    //Errno if something is wrong
    std::atomic_bool done{false};
    int fd{-1}; //File descriptor corresponding to the initialized instance of inotify
    int wd{-1}; //Watch descriptor corresponding to the initialized instance of inotify
    std::thread watching_thread;
    void create_inotify();
    void watching();
    void read_handle_event(const int);
    void refresh_song_list() const;
public:
    Inotify_module() { };
    explicit Inotify_module(const std::string&); //Argument is a path to folder to watch for
    explicit Inotify_module(const char* folder): Inotify_module(std::string(folder)) { };    //Argument is a path to folder to watch for
    //explicit Inotify_module(const parser_inotify)
    ~Inotify_module();

    bool if_no_error() const { return if_success; };
    bool if_no_error(std::string&) const;  //Overloaded variant with argument is to store symbolic name of possible error if it's occured
    void start_watching() { watching_thread = std::thread([this] () { this->watching(); }); }
    void stop_watching() { done = true; }
    void add_folder(std::string);
};

#endif // INOTIFY_MODULE_H
