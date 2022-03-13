#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <exception>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"

class c_s_exception : public std::exception
{
public:
    const char* what() const noexcept { return "File pet_project_config.ini not founded"; }
};


class Config_searching
{
private:
    std::string path;
public:
    Config_searching(const char*);
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
    Parser() { }
    Parser(const char*, const char*);
    const char* v_r(std::vector<std::string>::size_type index) { return values[index].c_str(); }
};

#endif // PARSER_H
