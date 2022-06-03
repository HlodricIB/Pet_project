#include "logger.h"

int main()
{
    std::shared_ptr<Logger> l_i = std::make_shared<Logger>("Inotify", "/home/nikita/C++/Pet_project/logs_folder/inotify_logs");
    if (l_i->whether_started())
    {
        l_i->make_record("Rec");
    }
    return 0;
}
