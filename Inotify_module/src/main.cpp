#include <iostream>
#include "inotify_module.h"

int main()
{
    std::shared_ptr<Parser> parser_inotify = std::make_shared<Parser_Inotify>(Config_searching ("pet_project_config.ini"));
    Inotify_module i_m("/home/nikita/C++/Pet_project/songs_folder", "/home/nikita/C++/Pet_project/logs_folder/inotify_logs");
    if (!i_m.if_no_error())
    {
        std::string err;
        i_m.if_no_error(err);
        std::cout << err << std::endl;
    }
    i_m.start_watching();
    if (!i_m.if_no_error())
    {
        std::string err;
        i_m.if_no_error(err);
        std::cout << err << std::endl;
    }

    char c;
    std::cin.get(c);

    return 0;
}
