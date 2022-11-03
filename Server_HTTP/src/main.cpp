#include <iostream>
//#include <vector>
#include "server_http.h"

int main()
{
    Config_searching path_to_config;
    try {
        path_to_config = Config_searching("pet_project_config.ini");
    }  catch (const c_s_exception& e) {
        std::cout << e.what() << std::endl;
    }
    using namespace server_http;
    auto parser_server_http = std::make_shared<Parser_Server_HTTP>(path_to_config.return_path());
    /*This works with handler that works directly with dir
    auto srvr_hlpr_clss = std::make_shared<Srvr_hlpr_clss>(parser_server_http, false, false, true, true);*/
    //This works with handler that works with database
    auto parser_DB_module = std::make_shared<Parser_DB>(path_to_config.return_path());
    auto module_DB = std::make_shared<DB_module>(parser_DB_module);
    auto srvr_hlpr_clss = std::make_shared<Srvr_hlpr_clss>(parser_server_http, false, false, true, false);
    Server_HTTP server_http{srvr_hlpr_clss};
    auto server_HTTP_handler = std::make_shared<Server_HTTP_handler>(module_DB, srvr_hlpr_clss->get_logger());
    srvr_hlpr_clss->set_handler(server_HTTP_handler);
    server_http.run();

    /*std::string command{"LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; SELECT count(*) FROM song_table"};
    const char* conninfo = "dbname = pet_project_db";
    auto connect = PQconnectdb(conninfo);
    //shared_PG_result res = std::make_shared<PG_result>(PQexec(connect, command.c_str()), PQdb(connect));
    auto res = PQexec(connect, command.c_str());
    auto n = PQntuples(res);
    auto r = std::atoi(PQgetvalue(res, 0, 0));
    //int n = res->get_rows_number();
    //using result_container = std::vector<std::vector<std::pair<const char*, size_t>>>;
    //result_container res_cntnr(res->get_result_container());*/
    return 0;
}
