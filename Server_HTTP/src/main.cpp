#include <iostream>
#include <vector>
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
    std::shared_ptr<Parser> parser_server_http = std::make_shared<Parser_Server_HTTP>(path_to_config.return_path());
    auto srvr_hlpr_clss = std::make_shared<Srvr_hlpr_clss>(parser_server_http, false, false, true, true, false);
    Server_HTTP server_http{srvr_hlpr_clss};
    server_http.run();
    return 0;
}
