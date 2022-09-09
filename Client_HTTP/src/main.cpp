#include "client_http.h"

int main()
{
    Config_searching path_to_config;
    try {
        path_to_config = Config_searching("pet_project_config.ini");
    }  catch (const c_s_exception& e) {
        std::cout << e.what() << std::endl;
    }
    using namespace client_http;
    std::shared_ptr<Parser> parser_client_http = std::make_shared<Parser_Client_HTTP>(path_to_config.return_path());
    Client_HTTP client_http{parser_client_http};
    client_http.run();
    return 0;
}
