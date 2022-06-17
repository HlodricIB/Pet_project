#include <iostream>
#include "inotify_module.h"
#include "handler.h"

int main()
{
    Config_searching config("pet_project_config.ini");
    std::shared_ptr<Parser> parser_DB_shrd_ptr = std::make_shared<Parser_DB>(config);
    std::shared_ptr<Parser> parser_inotify_shrd_ptr = std::make_shared<Parser_Inotify>(config);
    if (!parser_DB_shrd_ptr->parsed_info_ptr() || !parser_inotify_shrd_ptr->parsed_info_ptr())
    {
        std::cerr << "Error while parsing ini file\n";
        return 1;
    }
    auto c_pool_shrd_ptr = std::make_shared<connection_pool>(8, parser_DB_shrd_ptr);
    auto t_pool_DB_shrd_ptr = std::make_shared<thread_pool>(6);
    auto t_pool_Handler_shrd_ptr = std::make_shared<thread_pool>(2);
    auto DB_module_shrd_ptr = std::make_shared<DB_module>(c_pool_shrd_ptr, t_pool_DB_shrd_ptr);
    std::shared_ptr<Handler> handler_Inotify_DB_shrd_ptr = std::make_shared<Inotify_DB_handler>(DB_module_shrd_ptr, t_pool_Handler_shrd_ptr);
    auto Inotify_module_shrd_ptr = std::make_shared<Inotify_module>(parser_inotify_shrd_ptr);
    Inotify_module_shrd_ptr->set_handler(handler_Inotify_DB_shrd_ptr);
    Inotify_module_shrd_ptr->refresh_file_list();
    DB_module_shrd_ptr->warming_connections("dbname = pet_project_db", {"song_table", "log_table"});
    if (!Inotify_module_shrd_ptr->if_no_error())
    {
        std::string err;
        Inotify_module_shrd_ptr->if_no_error(err);
        std::cout << err << std::endl;
    }
    Inotify_module_shrd_ptr->start_watching();
    if (!Inotify_module_shrd_ptr->if_no_error())
    {
        std::string err;
        Inotify_module_shrd_ptr->if_no_error(err);
        std::cout << err << std::endl;
    }

    char c;
    std::cin.get(c);
    return 0;
}
