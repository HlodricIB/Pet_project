#ifndef INOTIFY_MODULE_H
#define INOTIFY_MODULE_H

#include <sys/inotify.h>
#include <string>
#include <string.h>
#include <errno.h>

class Inotify_module
{
private:
    bool initialized{false};    //Flag that shows if inotify was initialized successfully
    int error_number{0};    //Errno if initializing inotify was not successful
    int fd{-1}; //File descriptor corresponding to the initialized instance of inotify
    int wd{-1}; //Watch descriptor corresponding to the initialized instance of inotify
    void start_watching();
public:
    //Inotify_module();
    explicit Inotify_module(std::string folder): Inotify_module(folder.c_str()) { };
    explicit Inotify_module(const char*);

    bool is_initialized() const { return initialized; };
    bool is_initialized(std::string&) const;  //Overloaded variant with argument is to store symbolic name of possible error if it's occured
};

#endif // INOTIFY_MODULE_H
