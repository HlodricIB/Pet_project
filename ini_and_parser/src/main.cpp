#include <iostream>
#include "parser.h"

int main(int argc, char *argv[])
{
    Parser parser;
    if (argc > 1)
        parser = Parser(argv[1], "DB_module");
    else
    {
        try {
            Config_searching path_to_config("pet_project_config.ini");
            parser = Parser(path_to_config.return_path(), "DB_module");
        }  catch (const c_s_exception& e ) {
            std::cout << e.what() << std::endl;
        }
    }
    for (std::vector<std::string>::size_type i = 0; i < 8; ++i)
        std::cout << parser.v_r(i) << std::endl;
    std::vector<std::string> values;
    return 0;
}
