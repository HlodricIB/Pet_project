#include <iostream>
#include <memory>
#include "parser.h"
#include "DB_module.h"

int main(int argc, char *argv[])
{
    std::shared_ptr<Parser> parser;
    if (argc > 1)
        parser = std::make_shared<Parser_DB>(argv[1]);
    else
    {
        try {
            Config_searching path_to_config("pet_project_config.ini");
            parser = std::make_shared<Parser_DB>(path_to_config.return_path());
        }  catch (const c_s_exception& e ) {
            std::cout << e.what() << std::endl;
        }
    }
    parser->display();
    DB_module db(parser);
    db.exec_command("SELECT * FROM song_table");
    db.exec_command("SELECT * FROM log_table");
    return 0;
}
