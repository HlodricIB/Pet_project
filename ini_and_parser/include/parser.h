#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <exception>
#include <memory>
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
    boost::property_tree::ptree section_ptree;
    char** keywords;
public:
    Parser() { }
    Parser(const char*, const char*);
    ~Parser();
    void parsed_info() const;
    char** parsed_info_ptr() const { return keywords; };
};

#endif // PARSER_H
