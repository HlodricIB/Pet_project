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
    std::shared_ptr<char> config_to_find{nullptr};
    std::shared_ptr<char> path{nullptr};
    void searching_begin();
    void searching_forward (const char*, const char*);
    void searching_backward (const char*);
    void founded (const char*, const char*);
public:
    Config_searching() { }
    Config_searching(const char*);
    Config_searching(const std::string);
    const char* return_path() const { return path.get(); }
};

class Parser
{
public:
    virtual const char* const* parsed_info_ptr(char m = 'k') const = 0;
    virtual void display() const = 0;
};

class Parser_DB : public Parser
{
private:
    unsigned long size_char_ptr_ptr{0};
    char** keywords{0};
    char** values{0};
    void constructing_massives(const boost::property_tree::ptree&);
    void copying_massives(const Parser_DB&);
    void clearing_massives();
public:
    Parser_DB() { };
    Parser_DB(const char* config_filename);
    Parser_DB(const boost::property_tree::ptree& section_ptree_);
    Parser_DB(const Parser_DB&);
    Parser_DB& operator=(const Parser_DB&);
    ~Parser_DB();
    const char* const* parsed_info_ptr(char m = 'k') const override;
    void display() const override;
};

#endif // PARSER_H
