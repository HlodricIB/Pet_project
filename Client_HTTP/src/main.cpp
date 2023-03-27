#include "client_http.h"

int main()
{
    ::parser::Config_searching path_to_config;
    try {
        path_to_config = ::parser::Config_searching("pet_project_config.ini");
    }  catch (const ::parser::c_s_exception& e) {
        std::cout << e.what() << std::endl;
    }
    using namespace client_http;
    std::shared_ptr<::parser::Parser> parser_client_http = std::make_shared<::parser::Parser_Client_HTTP>(path_to_config.return_path());
    Client_HTTP client_http{parser_client_http};
    client_http.run();
    return 0;
}
