#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <memory>
#include "DB_module.h"

class Handler
{
public:
    Handler() { }
    virtual bool handle(const std::vector<std::string>&) = 0;
    virtual ~Handler() { };
};

class Inotify_DB_handler : public Handler
{
private:
    std::shared_ptr<DB_module> DB_ptr{nullptr};
    std::shared_ptr<thread_pool> t_pool_ptr{nullptr};
    void exec_command(const std::string&);
public:
    Inotify_DB_handler(std::shared_ptr<DB_module> DB_ptr_, std::shared_ptr<thread_pool> t_pool_ptr_): DB_ptr(DB_ptr_), t_pool_ptr(t_pool_ptr_) { }
    virtual bool handle(const std::vector<std::string>&) override;
};

#endif // HANDLER_H
