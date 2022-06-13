
#include "handler.h"



int main()
{
    std::shared_ptr<Handler> DB_module_handler = std::make_shared<Inotify_DB_handler>(std::make_shared<F>());
    DB_module_handler->handle({"add", "song1"});
    DB_module_handler->handle({"delete", "song2"});
    return 0;
}
