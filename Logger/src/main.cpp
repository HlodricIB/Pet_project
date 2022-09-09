#include "logger.h"

int main()
{
    std::shared_ptr<Logger> l_i = std::make_shared<Logger>("Server_HTTP", "/home/nikita/C++/Pet_project/logs_folder/server_http_logs");
    if (l_i->whether_started())
    {
        l_i->make_record("Rec");
    }
    return 0;
}
