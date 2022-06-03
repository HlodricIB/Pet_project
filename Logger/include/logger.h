#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <filesystem>
#include <fstream>

class Logger
{
private:
    std::string service;    //Name of logging service
    std::filesystem::path log_file;
    std::fstream f_stream;
    std::uintmax_t max_log_file_size{1000};   //Max size for log file, if we reach that size, create a new file
    int n{1};  //Number of next log file that will be created in log dir
    bool is_started{false};
    void open_file(const std::string&);
    static bool compare(std::filesystem::directory_entry const&, std::filesystem::directory_entry const&);
public:
    explicit Logger(const std::string&, const std::string&);
    Logger(const Logger&) = delete;
    Logger(Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&) = delete;
    ~Logger();
    void make_record(const std::string&);
    bool whether_started() { return is_started; }
};

#endif // LOGGER_H
