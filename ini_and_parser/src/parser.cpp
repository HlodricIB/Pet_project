#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include "parser.h"

Config_searching::Config_searching(const char* config_to_find_): config_to_find(config_to_find_)
{
    char currentDir[1024];
    //getcwd(currentDir, sizeof(currentDir));
    char previousDir[1024];
    //searching_forward(currentDir, previousDir);
    //std::strcpy(previousDir, currentDir);
    while (!config_founded && level_up <= 3)
    {
        getcwd(currentDir, sizeof(currentDir));
        {
            //std::cout  << "\n" << currentDir << " /currentDir/\n";
            //std::cout << previousDir << " /previousDir/\n" << std::endl;
        }
        searching_forward(currentDir, previousDir);
        std::strcpy(previousDir, currentDir);
        ++level_up;
    }
    if (!config_founded)
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
    struct dirent* entry;
    DIR* dir = opendir(currentDir);
    while (!config_founded && (entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            std::string current_taken_dir = std::string(currentDir) + "/" + entry->d_name;
            if (strcmp(entry->d_name, ".") != 0
                    && strcmp(entry->d_name, "..") != 0 && level_down <= 3 && strcmp(current_taken_dir.c_str(), previousDir) != 0)
            {
                ++level_down;
                chdir(entry->d_name);
                char currentDir[1024];
                getcwd(currentDir, sizeof(currentDir));
                {
                    char tcurrentDir[1024];
                    getcwd(tcurrentDir, sizeof(tcurrentDir));
                    std::cout << tcurrentDir << " /tcurrentDir/" << std::endl;
                }
                searching_forward(currentDir, previousDir);
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
