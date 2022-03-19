#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <exception>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"

class c_s_exception : public std::exception
{
private:
    std::string error_message;
public:
    c_s_exception(const char* error_message_): error_message(error_message_) { }
    const char* what() const noexcept { return error_message.c_str(); }
};


class Config_searching
{
private:
    int level_down{0};
    int level_up{0};
    bool config_founded{false};
    const char* config_to_find;
    std::string path;
public:
    Config_searching(const char*);
    void searching_forward (const char*, const char*);
    void searching_backward (char*);
    const char* return_path() { return path.c_str(); }
};

class Parser
{
private:
    //Order of keywords in pet_project_config.ini corresponding to order of values in
    //std::vector<std::string> values:
    //host, hostaddr, port, dbname, password, connect timeout, client encoding, sslmode
    std::vector<std::string> values;
public:
    Parser(int n) { values.reserve(n); }
    Parser(const char*, const char*, int);
    const char* parsed_info_index(std::vector<std::string>::size_type index) const { return values[index].c_str(); }
};

#endif // PARSER_H
