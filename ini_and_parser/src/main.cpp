#include <iostream>
#include "parser.h"

int main(int argc, char *argv[])
{
    using namespace parser;
    std::shared_ptr<Parser> parser_db = std::make_shared<Parser_DB>();
    std::shared_ptr<Parser> parser_inotify = std::make_shared<Parser_Inotify>();
    std::shared_ptr<Parser> parser_server_http = std::make_shared<Parser_Server_HTTP>();
    std::shared_ptr<Parser> parser_client_http = std::make_shared<Parser_Client_HTTP>();
    if (argc > 4)
    {
        parser_db = std::make_shared<Parser_DB>(argv[1]);
        parser_inotify = std::make_shared<Parser_Inotify>(argv[2]);
        parser_server_http = std::make_shared<Parser_Server_HTTP>(argv[3]);
        parser_client_http = std::make_shared<Parser_Client_HTTP>(argv[4]);
    } else
    {
        try {
            Config_searching path_to_config("pet_project_config.ini");
            parser_db = std::make_shared<Parser_DB>(path_to_config.return_path());
            parser_inotify = std::make_shared<Parser_Inotify>(path_to_config.return_path());
            parser_server_http = std::make_shared<Parser_Server_HTTP>(path_to_config.return_path());
            parser_client_http = std::make_shared<Parser_Client_HTTP>(path_to_config.return_path());
        }  catch (const c_s_exception& e) {
            std::cout << e.what() << std::endl;
        }
    }
    auto v_parser_db = parser_db->validate_parsed();
    auto v_parser_inotify = parser_inotify->validate_parsed();
    auto v_parser_server_http = parser_server_http->validate_parsed();
    auto v_parser_client_http = parser_client_http->validate_parsed();
    if (!v_parser_db.first)
    {
        std::cout << v_parser_db.second << std::endl;
    }
    if (!v_parser_inotify.first)
    {
        std::cout << v_parser_inotify.second << std::endl;
    }
    if (!v_parser_server_http.first)
    {
        std::cout << v_parser_server_http.second << std::endl;
    }
    if (!v_parser_client_http.first)
    {
        std::cout << v_parser_client_http.second << std::endl;
    }
    parser_db->display();
    std::cout << '\n';
    parser_inotify->display();
    std::cout << '\n';
    parser_server_http->display();
    std::cout << '\n';
    parser_client_http->display();
    return 0;
}
