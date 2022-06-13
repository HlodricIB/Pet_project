#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <memory>
#include "DB_module.h"

class Handler
{
public:
    Handler() { }
    virtual void handle(const std::vector<std::string>&) = 0;
    virtual ~Handler() { };
};

class Inotify_DB_handler : public Handler
{
private:
    std::shared_ptr<DB_module> DB_ptr{nullptr};
    std::shared_ptr<thread_pool> t_pool_ptr{nullptr};
public:
    Inotify_DB_handler(std::shared_ptr<DB_module> DB_ptr_, std::shared_ptr<thread_pool> t_pool_ptr_ = nullptr): DB_ptr(DB_ptr_), t_pool_ptr(t_pool_ptr_) { }
    virtual void handle(const std::vector<std::string>&) override;
};

#endif // HANDLER_H
