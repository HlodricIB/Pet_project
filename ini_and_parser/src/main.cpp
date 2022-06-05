#include <iostream>
#include "parser.h"

int main(int argc, char *argv[])
{
    std::shared_ptr<Parser> parser_db;
    std::shared_ptr<Parser> parser_inotify = std::make_shared<Parser_Inotify>();
    if (argc > 2)
    {
        parser_db = std::make_shared<Parser_DB>(argv[1]);
        parser_inotify = std::make_shared<Parser_Inotify>(argv[2]);
    } else
    {
        try {
            Config_searching path_to_config("pet_project_config.ini");
            parser_db = std::make_shared<Parser_DB>(path_to_config.return_path());
            parser_inotify = std::make_shared<Parser_Inotify>(path_to_config.return_path());
        }  catch (const c_s_exception& e ) {
            std::cout << e.what() << std::endl;
        }
    }
    parser_db->display();
    parser_inotify->display();
    return 0;
}
