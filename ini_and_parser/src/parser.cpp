#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include "parser.h"

Config_searching::Config_searching(const char* config_to_find)
{
    char currentDir[1024];
    struct dirent* entry;

    bool config_founded(false);
    while (!config_founded)
    {
        getcwd(currentDir, sizeof(currentDir));
        DIR* dir = opendir(currentDir);
        chdir("../");
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type != DT_DIR)
            {
                if (strcmp(entry->d_name, config_to_find) == 0)
                {
                    path = std::string(currentDir) + '/' + std::string(entry->d_name);
                    config_founded = true;
                    break;
                }
            }
        }
        closedir(dir);
        if (strcmp(currentDir, "/") == 0 && !config_founded)
        {
            throw c_s_exception();
        }
    }
}

Parser::Parser(const char* config_filename, const char* section)
{
    boost::property_tree::ptree config;
    try {
        boost::property_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const boost::property_tree::ini_parser_error& error) {
        std::cout << error.what() << std::endl;
    }
    const boost::property_tree::ptree& DB = config.get_child(section);
    values.push_back(DB.get<std::string>("host"));
    values.push_back(DB.get<std::string>("hostaddr"));
    values.push_back(DB.get<std::string>("port"));
    values.push_back(DB.get<std::string>("dbname"));
    values.push_back(DB.get<std::string>("password"));
    values.push_back(DB.get<std::string>("connect_timeout"));
    values.push_back(DB.get<std::string>("client_encoding"));
    values.push_back(DB.get<std::string>("sslmode"));
}
