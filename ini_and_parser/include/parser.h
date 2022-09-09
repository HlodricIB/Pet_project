#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <filesystem>
#include <cinttypes>
#include <utility>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"
#include "boost/core/ignore_unused.hpp"

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
    void constructing_massives(const prop_tree::ptree&);
    void copying_massives(const Parser&);
    std::pair<bool, std::string_view> validate_parsed(const size_t, const char* const[]);
public:
    Parser& operator=(const Parser&);
    virtual ~Parser();
    virtual const char* const* parsed_info_ptr(char m = 'k') const { boost::ignore_unused(m); return values; };
    void display() const;
    virtual std::pair<bool, std::string_view> validate_parsed() = 0;
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
    //~Parser_DB() override { };
    const char* const* parsed_info_ptr(char m = 'k') const override;
    std::pair<bool, std::string_view> validate_parsed() override { return std::make_pair(true, std::string_view()); }
};

class Parser_Inotify : public Parser
{
private:
    constexpr static size_t expected_count{3};
    constexpr static const char* expected[expected_count]{"files_folder", "logs_folder", "max_log_file_size"};
    void constructing_massives(const prop_tree::ptree&);
public:
    Parser_Inotify() { };
    explicit Parser_Inotify(const char*);
    explicit Parser_Inotify(const std::string& config_filename): Parser_Inotify(config_filename.c_str()) { }
    explicit Parser_Inotify(const prop_tree::ptree&);
    explicit Parser_Inotify(const Config_searching& c_s): Parser_Inotify(c_s.return_path().c_str()) { } //Not tested yet!!!
    explicit Parser_Inotify(const Parser_Inotify&);
    //~Parser_Inotify() override { };
    std::pair<bool, std::string_view> validate_parsed() override;
};

class Parser_Server_HTTP : public Parser
{
private:
    constexpr static int expected_count{7};
    constexpr static const char* expected[expected_count]{"address", "port", "server_name", "files_folder", "logs_folder", "max_log_file_size", "num_threads"};
    void constructing_massives(const prop_tree::ptree&);
public:
    Parser_Server_HTTP() { };
    explicit Parser_Server_HTTP(const char*);
    explicit Parser_Server_HTTP(const std::string& config_filename): Parser_Server_HTTP(config_filename.c_str()) { }
    explicit Parser_Server_HTTP(const prop_tree::ptree&);
    explicit Parser_Server_HTTP(const Config_searching& c_s): Parser_Server_HTTP(c_s.return_path().c_str()) { } //Not tested yet!!!
    explicit Parser_Server_HTTP(const Parser_Server_HTTP&);
    //~Parser_Server_HTTP() override { };
    std::pair<bool, std::string_view> validate_parsed() override;
};

class Parser_Client_HTTP : public Parser
{
private:
    constexpr static int expected_count{5};
    constexpr static const char* expected[expected_count]{"host(address)", "port(service)", "client_name", "version", "num_threads"};
    void constructing_massives(const prop_tree::ptree&);
public:
    Parser_Client_HTTP() { };
    explicit Parser_Client_HTTP(const char*);
    explicit Parser_Client_HTTP(const std::string& config_filename): Parser_Client_HTTP(config_filename.c_str()) { }
    explicit Parser_Client_HTTP(const prop_tree::ptree&);
    explicit Parser_Client_HTTP(const Config_searching& c_s): Parser_Client_HTTP(c_s.return_path().c_str()) { } //Not tested yet!!!
    explicit Parser_Client_HTTP(const Parser_Client_HTTP&);
    //~Parser_Server_HTTP() override { };
    std::pair<bool, std::string_view> validate_parsed() override;
};

#endif // PARSER_H
