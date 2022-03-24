#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include "parser.h"

Config_searching::Config_searching(const char* config_to_find_)
{
    config_to_find = std::shared_ptr<char>(new char[std::strlen(config_to_find_) + 1]);
    std::strcpy(config_to_find.get(), config_to_find_);
    searching_begin();
}

Config_searching::Config_searching(const std::string config_to_find_)
{
    Config_searching(config_to_find_.c_str());
}

void Config_searching::searching_begin()
{
    char currentDir[1024];
    char previousDir[1024];
    while (!config_founded && level_up <= 3)
    {
        getcwd(currentDir, sizeof(currentDir));
        searching_forward(currentDir, previousDir);
        std::strcpy(previousDir, currentDir);
        ++level_up;
    }
    if (!config_founded)
        searching_backward(currentDir);
}

void Config_searching::founded(const char* currentDir, const char* d_name)
{
    auto currentDir_length = std::strlen(currentDir);
    auto d_name_length =  + std::strlen(d_name);
    auto p_length = currentDir_length + d_name_length +1;
    path = std::shared_ptr<char>(new char[p_length]);
    char* temp_path = path.get();
    std::strcpy(temp_path, currentDir);
    std::strcpy(temp_path + currentDir_length, "/");
    std::strcpy(temp_path + currentDir_length + 1, d_name);
}

void Config_searching::searching_backward(const char* currentDir)
{
    char* temp_config_to_find = config_to_find.get();
    struct dirent* entry;
    while (!config_founded)
    {
        DIR* dir = opendir(currentDir);
        chdir("../");
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type != DT_DIR)
            {
                if (strcmp(entry->d_name, temp_config_to_find) == 0)
                {
                    founded(currentDir, entry->d_name);
                    config_founded = true;
                    closedir(dir);
                    return;
                }
            }
        }
        closedir(dir);
        if (strcmp(currentDir, "/") == 0 && !config_founded)
        {
            std::string error_message = temp_config_to_find + std::string(" not founded");
            throw c_s_exception(error_message.c_str());
        }
    }
}

void Config_searching::searching_forward (const char* currentDir, const char* previousDir)
{
    char* temp_config_to_find = config_to_find.get();
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
                searching_forward(currentDir, previousDir);
                --level_down;
            }
        }
        else
        {
            if (strcmp(entry->d_name, temp_config_to_find) == 0)
            {
                founded(currentDir, entry->d_name);
                config_founded = true;
                closedir(dir);
                return;
            }
        }
    }

    chdir("../");
    closedir(dir);
}

Parser_DB::Parser_DB(const boost::property_tree::ptree& config)
{
    constructing_massives(config);
}

Parser_DB::Parser_DB(const char* config_filename)
{
    boost::property_tree::ptree config;
    try {
        boost::property_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const boost::property_tree::ini_parser_error& error) {
        std::cerr << error.what() << std::endl;
    }
    constructing_massives(config);
}

void Parser_DB::constructing_massives(const boost::property_tree::ptree& config)
{
    const boost::property_tree::ptree& section_ptree = config.get_child("DB_module");
    int char_i = 0;
    size_char_ptr_ptr = section_ptree.size();
    keywords = new char*[size_char_ptr_ptr + 1];
    values = new char*[size_char_ptr_ptr + 1];
    for (auto& i : section_ptree)
    {
        keywords[char_i] = new char[i.first.size()];
        values[char_i] = new char[i.second.data().size()];
        std::strcpy(keywords[char_i], i.first.c_str());
        std::strcpy(values[char_i], i.second.data().c_str());
        ++char_i;
    }
    keywords[char_i] = nullptr;
    values[char_i] = nullptr;
}

void Parser_DB::copying_massives(const Parser_DB& p)
{
    keywords = new char*[size_char_ptr_ptr + 1];
    values = new char*[size_char_ptr_ptr + 1];
    for (unsigned long i = 0; i != size_char_ptr_ptr; ++i)
    {
        keywords[i] = new char[strlen(p.keywords[i]) + 1];
        values[i] = new char[strlen(p.values[i]) + 1];
        std::strcpy(keywords[i], p.keywords[i]);
        std::strcpy(values[i], p.values[i]);
    }
    keywords[size_char_ptr_ptr] = nullptr;
    values[size_char_ptr_ptr] = nullptr;
}

void Parser_DB::clearing_massives()
{
    if (size_char_ptr_ptr !=0 )
    {
        for (unsigned long i = 0; i != size_char_ptr_ptr; ++i)
        {
            delete [] keywords[i];
            delete [] values[i];
        }
        delete [] keywords;
        delete [] values;
    }
}

Parser_DB::Parser_DB(const Parser_DB& p)
{
    size_char_ptr_ptr = p.size_char_ptr_ptr;
    copying_massives(p);
}

Parser_DB& Parser_DB::operator=(const Parser_DB& rhs)
{
    if (this == &rhs)
        return *this;    
    clearing_massives();
    size_char_ptr_ptr = rhs.size_char_ptr_ptr;
    copying_massives(rhs);
    return *this;
}

const char* const* Parser_DB::parsed_info_ptr(char m) const
{
    if ( m == 'k' || m == 'K')
        return keywords;
    if (m == 'v' || m == 'V')
        return values;
    return nullptr;
}

void Parser_DB::display() const
{
    for (unsigned long i = 0; i != size_char_ptr_ptr; ++i)
    {
        std::cout << keywords[i] << " = " << values[i] << std::endl;
    }
}

Parser_DB::~Parser_DB()
{
    clearing_massives();
}
