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
#include "parser.h"
#include "DB_module.h"
#include "handler.h"

class Inotify_module
{
private:
    std::string songs_folder;
    std::unique_ptr<Logger> logger{nullptr};
    bool if_success{false};    //Flag that shows if function works successfully
    int error_number{0};    //Errno if something is wrong
    std::atomic_bool done{false};
    int fd{-1}; //File descriptor corresponding to the initialized instance of inotify
    int wd{-1}; //Watch descriptor corresponding to the initialized instance of inotify
    std::thread watching_thread;
    std::shared_ptr<Handler> handler{0};
    bool create_dir_if_not_exist();
    void create_inotify();
    void watching();
    void read_handle_event(const int);
    void refresh_song_list() const;
public:
    Inotify_module() { };
    Inotify_module(const std::string&, const std::string&); //First argument is a path to folder to watch for, second is a path to log files dir
    Inotify_module(const char* songs_folder, const char* logs_folder): Inotify_module(std::string(songs_folder), std::string(logs_folder)) { };    //Argument is a path to folder to watch for, second is a path to log files dir
    explicit Inotify_module(std::shared_ptr<Parser> parser_inotify): Inotify_module((parser_inotify->parsed_info_ptr())[0],
                                                                     std::make_unique<Logger>("Inotify", (parser_inotify->parsed_info_ptr())[1], std::strtoumax((parser_inotify->parsed_info_ptr())[2], nullptr, 10))) { }
    Inotify_module(const std::string&, std::unique_ptr<Logger>); //First argument is a path to folder to watch for
    Inotify_module(const Inotify_module&) = delete;
    Inotify_module(Inotify_module&) = delete;
    Inotify_module& operator=(const Inotify_module&) = delete;
    Inotify_module& operator=(Inotify_module&) = delete;
    ~Inotify_module();

    bool if_no_error() const { return if_success; };
    bool if_no_error(std::string&) const;  //Overloaded variant with argument is to store symbolic name of possible error if it's occured
    void start_watching() { watching_thread = std::thread([this] () { this->watching(); }); }
    void stop_watching() { done = true; }
    void set_folder(std::string);   //To set folder to watch for
    void set_handler(std::shared_ptr<Handler> handler_) { handler = handler_; } //To set events handler
};



#endif // INOTIFY_MODULE_H
