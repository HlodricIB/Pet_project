#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <filesystem>
#include <fstream>

class Logger
{
private:
    std::string service;    //Name of service for logging
    std::filesystem::path log_file;
    std::uintmax_t max_log_file_size{0};   //Max size for log file, if we reach that size, create a new file
    std::fstream f_stream;
    int n{1};  //Number of next log file that will be created in log dir
    bool is_started{false};
    bool create_dir_if_not_exist(const std::filesystem::path&);
    void open_file(const std::string&);
    static bool compare(std::filesystem::directory_entry const&, std::filesystem::directory_entry const&);
public:
    explicit Logger(const std::string&, const std::string&, std::uintmax_t size = 1000);    //First argument is a name of service for
                //logging, second is a path to log files dir, third is a max size for log file, if we reach that size, create a new file
    explicit Logger(const char* service, const char* log_dir, std::uintmax_t size = 1000): Logger(std::string(service), std::string(log_dir), size) { }   //First argument
                //is a name of service for logging, second is a path to log files dir, third is a max size for log file, if we reach that size, create a new file
    Logger(const Logger&) = delete;
    Logger(Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&) = delete;
    ~Logger();
    void make_record(const std::string&);
    bool whether_started() { return is_started; }
};

#endif // LOGGER_H
