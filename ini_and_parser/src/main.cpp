#include <iostream>
#include "parser.h"

int main(int argc, char *argv[])
{
    Parser_DB parser{};
    if (argc > 1)
        parser = Parser_DB(argv[1]);
    else
    {
        try {
            Config_searching path_to_config("pet_project_config.ini");
            parser = Parser_DB(path_to_config.return_path());
        }  catch (const c_s_exception& e ) {
            std::cout << e.what() << std::endl;
        }
    }
    parser.display();
    return 0;
}
