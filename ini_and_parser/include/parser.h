#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <filesystem>
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

namespace fs = std::filesystem;

class Config_searching
{
private:
    int level_down{0};
    int level_up{0};
    bool config_founded{false};
    fs::path config_to_find;
    std::string path;
    void searching_begin();
    void searching_forward (fs::path&);
    void searching_backward ();
public:
    Config_searching() { }
    Config_searching(const char*);
    Config_searching(const std::string config_to_find_);
    std::string return_path() const { return path; }
};

namespace prop_tree = boost::property_tree;

class Parser
{
public:
    Parser() { }
    virtual ~Parser() { }
    virtual const char* const* parsed_info_ptr(char m = 'k') const = 0;
    virtual void display() const = 0;
};

class Parser_DB : public Parser
{
private:
    size_t size_char_ptr_ptr{0};
    char** keywords{nullptr};
    char** values{nullptr};
    void constructing_massives(const prop_tree::ptree&);
    void copying_massives(const Parser_DB&);
    void clearing_massives();
public:
    Parser_DB() { };
    Parser_DB(const char* config_filename);
    Parser_DB(const std::string config_filename): Parser_DB(config_filename.c_str()) { }
    Parser_DB(const prop_tree::ptree& section_ptree_);
    Parser_DB(const Parser_DB&);
    Parser_DB& operator=(const Parser_DB&);
    ~Parser_DB() override;
    const char* const* parsed_info_ptr(char m = 'k') const override;
    void display() const override;
};

#endif // PARSER_H
