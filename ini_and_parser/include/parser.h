#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <filesystem>
#include <cinttypes>
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
    explicit Config_searching(const char*);
    explicit Config_searching(const std::string config_to_find_);
    std::string return_path() const { return path; }
};

namespace prop_tree = boost::property_tree;

class Parser
{
private:
    void clearing_massives();
protected:
    size_t size_char_ptr_ptr{0};
    char** keywords{nullptr};
    char** values{nullptr};
    void copying_massives(const Parser&);
public:
    Parser& operator=(const Parser&);
    virtual ~Parser();
    virtual const char* const* parsed_info_ptr(char m = 'k') const = 0;
    virtual void display() const = 0;
};

class Parser_DB : public Parser
{
private:
    void constructing_massives(const prop_tree::ptree&);
public:
    Parser_DB() { };
    explicit Parser_DB(const char*);
    explicit Parser_DB(const std::string config_filename): Parser_DB(config_filename.c_str()) { }
    explicit Parser_DB(const prop_tree::ptree&);
    explicit Parser_DB(const Config_searching& c_s): Parser_DB(c_s.return_path().c_str()) { }   //Not tested yet!!!!
    explicit Parser_DB(const Parser_DB&);
    ~Parser_DB() override { };
    const char* const* parsed_info_ptr(char m = 'k') const override;
    void display() const override;
};

class Parser_Inotify : public Parser
{
private:
    void constructing_massives(const prop_tree::ptree&);
public:
    Parser_Inotify() { };
    explicit Parser_Inotify(const char*);
    explicit Parser_Inotify(const std::string& config_filename): Parser_Inotify(config_filename.c_str()) { }
    explicit Parser_Inotify(const prop_tree::ptree&);
    explicit Parser_Inotify(const Config_searching& c_s): Parser_Inotify(c_s.return_path().c_str()) { } //Not tested yet!!!
    explicit Parser_Inotify(const Parser_Inotify&);
    ~Parser_Inotify() override { };
    const char* const* parsed_info_ptr(char) const override { return values; };
    void display() const override;
};

#endif // PARSER_H
