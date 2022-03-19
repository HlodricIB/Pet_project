#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include "parser.h"

Config_searching::Config_searching(const char* config_to_find_): config_to_find(config_to_find_)
{
    char currentDir[1024];
    getcwd(currentDir, sizeof(currentDir));
    char previousDir[1024];
    std::strcpy(previousDir, currentDir);
    //char new_previousDir[1024];
    //char startDir[1024];
    {
        std::cout << "\n" << std::endl;
        std::cout << currentDir << std::endl;
        std::cout << previousDir << std::endl;
        std::cout << "\n" << std::endl;
    }
    searching_forward(currentDir, previousDir);
    {
        std::cout << "\n" << std::endl;
        std::cout << currentDir << std::endl;
        std::cout << previousDir << std::endl;
        std::cout << "\n" << std::endl;
    }
    while (level_up <= 3)
    {
        getcwd(currentDir, sizeof(currentDir));
        {
            std::cout << "\n" << std::endl;
            std::cout << currentDir << std::endl;
            std::cout << previousDir << std::endl;
            std::cout << "\n" << std::endl;
        }
        searching_forward(currentDir, previousDir);
        {
            std::cout << "\n" << std::endl;
            std::cout << currentDir << std::endl;
            std::cout << previousDir << std::endl;
            std::cout << "\n" << std::endl;
        }
        std::strcpy(previousDir, currentDir);
        {
            getcwd(currentDir, sizeof(currentDir));
            std::cout << "\n" << std::endl;
            std::cout << currentDir << std::endl;
            std::cout << previousDir << std::endl;
            std::cout << "\n" << std::endl;
        }
        //if (level_down == 0)
            ++level_up;
    }
    searching_backward(currentDir);
}

void Config_searching::searching_backward(char* currentDir)
{
    struct dirent* entry;
    while (!config_founded)
    {
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
                    closedir(dir);
                    return;
                }
            }
        }
        closedir(dir);
        if (strcmp(currentDir, "/") == 0 && !config_founded)
        {
            throw c_s_exception("Pet_project_config.ini not founded");
        }
    }
}

void Config_searching::searching_forward (const char* currentDir, const char* previousDir)
{
    //if (level_down > 3 || config_founded)
    //    return;
    struct dirent* entry;
    DIR* dir = opendir(currentDir);
    while ((entry = readdir(dir)) != NULL)
    {
        {
            //std::cout << entry->d_name << std::endl;
        }
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") != 0
                    && strcmp(entry->d_name, "..") != 0 && level_down <= 3)
            {
                ++level_down;
                chdir(entry->d_name);
                char currentDir[1024];
                getcwd(currentDir, sizeof(currentDir));
                if (strcmp(currentDir, previousDir) != 0)
                {
                    {
                        char tcurrentDir[1024];
                        getcwd(tcurrentDir, sizeof(currentDir));
                        std::cout << tcurrentDir << std::endl;
                    }
                    searching_forward(currentDir, previousDir);
                }
                --level_down;
            }
        }
        else
        {
            if (strcmp(entry->d_name, config_to_find) == 0)
            {
                path = std::string(currentDir) + '/' + std::string(entry->d_name);
                config_founded = true;
                closedir(dir);
                return;
            }
        }
    }
    chdir("../");
    closedir(dir);
    {
        std::cout << "\n" << currentDir << " changed to upper dir" << std::endl;
        std::cout << currentDir << " closed\n" << std::endl;
    }
}

Parser::Parser(const char* config_filename, const char* section, int n)
{
    boost::property_tree::ptree config;
    try {
        boost::property_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const boost::property_tree::ini_parser_error& error) {
        std::cout << error.what() << std::endl;
    }
    const boost::property_tree::ptree& DB = config.get_child(section);
    values.reserve(n);
    values.push_back(DB.get<std::string>("host"));
    values.push_back(DB.get<std::string>("hostaddr"));
    values.push_back(DB.get<std::string>("port"));
    values.push_back(DB.get<std::string>("dbname"));
    values.push_back(DB.get<std::string>("password"));
    values.push_back(DB.get<std::string>("connect_timeout"));
    values.push_back(DB.get<std::string>("client_encoding"));
    values.push_back(DB.get<std::string>("sslmode"));
}
