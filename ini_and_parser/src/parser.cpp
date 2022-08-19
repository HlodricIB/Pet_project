#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include "parser.h"

Config_searching::Config_searching(const char* config_to_find_)
{
    config_to_find = fs::path(config_to_find_);
    searching_begin();
}

Config_searching::Config_searching(const std::string config_to_find_)
{
    config_to_find = fs::path(config_to_find_);
    searching_begin();
}

void Config_searching::searching_begin()
{
    fs::path previousDir;
    while (!config_founded && level_up <= 3)
    {
        searching_forward(previousDir);
        fs::path previousDir = fs::current_path();
        ++level_up;
    }
    if (!config_founded)
    {
        searching_backward();
    }
}

void Config_searching::searching_backward()
{
    while (true)
    {
        fs::directory_iterator begin = fs::directory_iterator(fs::current_path());
        fs::directory_iterator end = fs::directory_iterator();
        while (begin != end)
        {
            if (fs::is_regular_file(begin->path()) && ((begin->path()).filename() == config_to_find))
            {
                path = fs::current_path().string();
                config_founded = true;
                return;
            }
            ++begin;
        }
        if (fs::current_path() == "/")
        {
            std::string error_message = config_to_find.string() + std::string(" not founded");
            throw c_s_exception(error_message.c_str());
        }
        fs::current_path(fs::current_path() / "..");
    }
}

void Config_searching::searching_forward (fs::path& previousDir)
{
    fs::path currentDir = fs::current_path();
    fs::path upperDir = currentDir / "..";
    fs::path same_Dir = currentDir / ".";
    fs::directory_iterator begin = fs::directory_iterator(currentDir);
    fs::directory_iterator end = fs::directory_iterator();
    while (begin != end)
    {
        fs::path curr_dir_object = begin->path();
        if (fs::is_directory(curr_dir_object))
        {

            if (curr_dir_object != upperDir && curr_dir_object != same_Dir && level_down <= 3 && curr_dir_object != previousDir)
            {
                ++level_down;
                fs::current_path(curr_dir_object);
                searching_forward(previousDir);
                --level_down;
            }
        }
        else
        {
            if (config_founded)
            {
                return;
            }

            if (fs::is_regular_file(curr_dir_object) && (curr_dir_object).filename() == config_to_find)
            {
                path = curr_dir_object.string();
                config_founded = true;
                return;
            }
        }
        ++begin;
    }
    if (fs::current_path() == "/")
    {
        std::string error_message = config_to_find.string() + std::string(" not founded");
        throw c_s_exception(error_message.c_str());
    }
    fs::current_path(upperDir);
}

Parser::~Parser()
{
    clearing_massives();
}

void Parser::clearing_massives()
{
    if (size_char_ptr_ptr != 0)
    {
        for (size_t i = 0; i != size_char_ptr_ptr; ++i)
        {
            delete [] values[i];
        }
        if (keywords)
        {
            for (size_t i = 0; i != size_char_ptr_ptr; ++i)
            {
                delete [] keywords[i];
            }
        }
        delete [] keywords;
        delete [] values;
    }
}

void Parser::copying_massives(const Parser& p)
{
    values = new char*[size_char_ptr_ptr + 1];
    for (size_t i = 0; i != size_char_ptr_ptr; ++i)
    {
        auto values_size = strlen(p.values[i]) + 1;
        values[i] = new char[values_size];
        std::strncpy(values[i], p.values[i], values_size);
    }
    values[size_char_ptr_ptr] = nullptr;
    if (p.keywords)
    {
        keywords = new char*[size_char_ptr_ptr + 1];
        for (size_t i = 0; i != size_char_ptr_ptr; ++i)
        {
            auto keywords_size = strlen(p.keywords[i]) + 1;
            keywords[i] = new char[keywords_size];
            std::strncpy(keywords[i], p.keywords[i], keywords_size);
        }
        keywords[size_char_ptr_ptr] = nullptr;
    }
}

Parser& Parser::operator=(const Parser& rhs)
{
    if (this == &rhs)
    {
        return *this;
    }
    clearing_massives();
    size_char_ptr_ptr = rhs.size_char_ptr_ptr;
    copying_massives(rhs);
    return *this;
}

Parser_DB::Parser_DB(const prop_tree::ptree& config)
{
    constructing_massives(config);
}

Parser_DB::Parser_DB(const char* config_filename)
{
    prop_tree::ptree config;
    try {
        prop_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const prop_tree::ini_parser_error& error) {
        std::cerr << error.what() << std::endl;
    }
    constructing_massives(config);
}

void Parser_DB::constructing_massives(const prop_tree::ptree& config)
{
    const prop_tree::ptree& section_ptree = config.get_child("DB_module");
    int char_i = 0;
    size_char_ptr_ptr = section_ptree.size();
    keywords = new char*[size_char_ptr_ptr + 1];
    values = new char*[size_char_ptr_ptr + 1];
    for (const auto& i : section_ptree)
    {
        auto first_size = i.first.size() + 1;
        auto second_size = i.second.data().size() + 1;
        keywords[char_i] = new char[first_size];
        values[char_i] = new char[second_size];
        std::strncpy(keywords[char_i], i.first.c_str(), first_size);
        std::strncpy(values[char_i], i.second.data().c_str(), second_size);
        ++char_i;
    }
    keywords[char_i] = nullptr;
    values[char_i] = nullptr;
}

Parser_DB::Parser_DB(const Parser_DB& p)
{
    size_char_ptr_ptr = p.size_char_ptr_ptr;
    copying_massives(p);
}

const char* const* Parser_DB::parsed_info_ptr(char m) const
{
    switch (m)
    {
    case 'k':
        return keywords;
        break;
    case 'K':
        return keywords;
        break;
    case 'v':
        return values;
        break;
    case 'V':
        return values;
        break;
    default:
        return nullptr;
        break;
    }
}

void Parser_DB::display() const
{
    for (size_t i = 0; i != size_char_ptr_ptr; ++i)
    {
        std::cout << keywords[i] << " = " << values[i] << std::endl;
    }
}

Parser_Inotify::Parser_Inotify(const prop_tree::ptree& config)
{
    constructing_massives(config);
}

Parser_Inotify::Parser_Inotify(const char* config_filename)
{
    prop_tree::ptree config;
    try {
        prop_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const prop_tree::ini_parser_error& error) {
        std::cerr << error.what() << std::endl;
    }
    constructing_massives(config);
}

void Parser_Inotify::constructing_massives(const prop_tree::ptree& config)
{
    const prop_tree::ptree& section_ptree = config.get_child("Inotify_module");
    int char_i = 0;
    size_char_ptr_ptr = section_ptree.size();
    values = new char*[size_char_ptr_ptr + 1];
    std::vector<std::string> keys{"songs_folder", "logs_folder", "max_log_file_size"};
    prop_tree::ptree::const_assoc_iterator it;
    auto eq_end = section_ptree.not_found();
    for (const auto& i : keys)
    {
        it = section_ptree.find(i);
        if (it != eq_end)
        {
            std::string val = it->second.data();
            auto values_size = val.size() + 1;
            values[char_i] = new char[values_size];
            std::strncpy(values[char_i], val.c_str(), values_size);
            ++char_i;
        } else
        {
            std::cerr << i << " key in config file not found\n" << std::flush;
            values = nullptr;
            break;
        }

    }
}

Parser_Inotify::Parser_Inotify(const Parser_Inotify& p)
{
    size_char_ptr_ptr = p.size_char_ptr_ptr;
    copying_massives(p);
}

void Parser_Inotify::display() const
{
    for (size_t i = 0; i != size_char_ptr_ptr; ++i)
    {
        std::cout << values[i] << std::endl;
    }
}
