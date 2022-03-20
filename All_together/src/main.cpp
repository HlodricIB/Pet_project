#include <iostream>
#include "parser.h"

int main(int argc, char *argv[])
{
    Parser parser{};
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
    parser.parsed_info();
    auto keywords = parser.parsed_info_ptr();
    std::cout << "\n" << keywords[0] << std::endl;
    std::cout << keywords[1] << std::endl;
    std::cout << keywords[2] << std::endl;
    return 0;
}
