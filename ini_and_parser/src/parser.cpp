#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include "boost/optional.hpp"
#include "parser.h"

namespace parser
{
Config_searching::Config_searching(const char* config_to_find_)
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
        fs::directory_iterator begin = fs::directory_iterator(fs::current_path(), fs::directory_options::skip_permission_denied);
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
    fs::directory_iterator begin = fs::directory_iterator(currentDir, fs::directory_options::skip_permission_denied);
    fs::directory_iterator end = fs::directory_iterator();
    while (begin != end)
    {
        if (fs::is_regular_file(begin->path()))
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
                if ((curr_dir_object).filename() == config_to_find)
                {
                    path = curr_dir_object.string();
                    config_founded = true;
                    return;
                }
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

void Parser::constructing_massives(const prop_tree::ptree& section_ptree)
{
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

void Parser::display() const
{
    for (size_t i = 0; i != size_char_ptr_ptr; ++i)
    {
        std::cout << keywords[i] << " = " << values[i] << std::endl;
    }
}

std::pair<bool, std::string_view> Parser::validate_parsed(const size_t expected_count, const char* const expected[])
{
    if (size_char_ptr_ptr < expected_count)
    {
        return std::make_pair(false, std::string_view("Parsed count of keywords is less than it needed"));
    }
    for (size_t i = 0; i != size_char_ptr_ptr; ++i)
    {
        if (std::strcmp(keywords[i], expected[i]) != 0)
        {
            return std::make_pair(false, std::string_view(expected[i]));
        }
    }
    return std::make_pair(true, std::string_view());
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
    }  catch (const prop_tree::ptree_error& error) {
        std::cerr << error.what() << std::endl;
        throw error;
    }
    constructing_massives(config);
}

void Parser_DB::constructing_massives(const prop_tree::ptree& config)
{
    boost::optional<const prop_tree::ptree&> section_ptree = config.get_child_optional("DB_module");
    if (section_ptree)
    {
        Parser::constructing_massives(*section_ptree);
    } else {
        std::cerr << "No appropriate section (DB_module) in the provided structure" << std::endl;
    }
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

std::pair<bool, std::string_view> Parser_DB::validate_parsed()
{
    if (size_char_ptr_ptr > 0)
    {
        return std::make_pair(true, std::string_view());
    }
    return std::make_pair(false, std::string_view("No data was parsed, may be because there is no appropriate information to "
                                                                                                                "parse in ini-file"));
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
    }  catch (const prop_tree::ptree_error& error) {
        std::cerr << error.what() << std::endl;
        throw error;
    }
    constructing_massives(config);
}

Parser_Inotify::Parser_Inotify(const Parser_Inotify& p)
{
    size_char_ptr_ptr = p.size_char_ptr_ptr;
    copying_massives(p);
}

void Parser_Inotify::constructing_massives(const prop_tree::ptree& config)
{
    boost::optional<const prop_tree::ptree&> section_ptree = config.get_child_optional("Inotify_module");
    if (section_ptree)
    {
        Parser::constructing_massives(*section_ptree);
    } else {
        std::cerr << "No appropriate section (Inotify_module) in the provided structure" << std::endl;
    }
}

std::pair<bool, std::string_view> Parser_Inotify::validate_parsed()
{
    return Parser::validate_parsed(expected_count, expected);
}

Parser_Server_HTTP::Parser_Server_HTTP(const prop_tree::ptree& config)
{
    constructing_massives(config);
}

Parser_Server_HTTP::Parser_Server_HTTP(const char* config_filename)
{
    prop_tree::ptree config;
    try {
        prop_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const prop_tree::ptree_error& error) {
        std::cerr << error.what() << std::endl;
        throw error;
    }
    constructing_massives(config);
}

Parser_Server_HTTP::Parser_Server_HTTP(const Parser_Server_HTTP& p)
{
    size_char_ptr_ptr = p.size_char_ptr_ptr;
    copying_massives(p);
}

void Parser_Server_HTTP::constructing_massives(const prop_tree::ptree& config)
{
    boost::optional< const prop_tree::ptree&> section_ptree = config.get_child_optional("Server_HTTP");
    if (section_ptree)
    {
        Parser::constructing_massives(*section_ptree);
    } else {
        std::cerr << "No appropriate section (Server_HTTP) in the provided structure" << std::endl;
    }
}

std::pair<bool, std::string_view> Parser_Server_HTTP::validate_parsed()
{
    return Parser::validate_parsed(expected_count, expected);
}

Parser_Client_HTTP::Parser_Client_HTTP(const prop_tree::ptree& config)
{
    constructing_massives(config);
}

Parser_Client_HTTP::Parser_Client_HTTP(const char* config_filename)
{
    prop_tree::ptree config;
    try {
        prop_tree::ini_parser::read_ini(config_filename, config);
    }  catch (const prop_tree::ptree_error& error) {
        std::cerr << error.what() << std::endl;
        throw error;
    }
    constructing_massives(config);
}

Parser_Client_HTTP::Parser_Client_HTTP(const Parser_Client_HTTP& p)
{
    size_char_ptr_ptr = p.size_char_ptr_ptr;
    copying_massives(p);
}

void Parser_Client_HTTP::constructing_massives(const prop_tree::ptree& config)
{
    boost::optional< const prop_tree::ptree&> section_ptree = config.get_child_optional("Client_HTTP");
    if (section_ptree)
    {
        Parser::constructing_massives(*section_ptree);
    } else {
        std::cerr << "No appropriate section (Client_HTTP) in the provided structure" << std::endl;
    }
}

std::pair<bool, std::string_view> Parser_Client_HTTP::validate_parsed()
{
    return Parser::validate_parsed(expected_count, expected);
}
}   //namespace parser
