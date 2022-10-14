#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <filesystem>
#include <fstream>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

class Logger
{
private:
    std::string service;    //Name of service for logging
    std::filesystem::path log_file;
    std::uintmax_t max_log_file_size{0};   //Max size for log file, if we reach that size, create a new file
    int n{1};  //Number of next log file that will be created in log dir
    bool is_started{false};
    bool create_dir_if_not_exist(const std::filesystem::path&);
    void open_file(const std::string&);
    static bool compare(std::filesystem::directory_entry const&, std::filesystem::directory_entry const&);
protected:
    std::fstream f_stream;
public:
    Logger(const std::string&, const std::string&, std::uintmax_t size = 1000);    //First argument is a name of service for
                //logging, second is a path to log files dir, third is a max size for log file, if we reach that size, create a new file
    Logger(const char* service, const char* log_dir, std::uintmax_t size = 1000): Logger(std::string(service), std::string(log_dir), size) { }   //First argument
                //is a name of service for logging, second is a path to log files dir, third is a max size for log file, if we reach that size, create a new file
    Logger(const Logger&) = delete;
    Logger(Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&) = delete;
    ~Logger();
    virtual void make_record(const std::string&);
    bool whether_started() { return is_started; }
};

class Logger_srvr_http : public Logger
{
private:
    boost::asio::strand<boost::asio::io_context::executor_type> _strand;
public:
    Logger_srvr_http(boost::asio::io_context& ioc, const std::string& service, const std::string& log_dir,
                              std::uintmax_t size = 1000): Logger(service, log_dir, size), _strand(ioc.get_executor()) { }  //First argument presence explanation: strand object may be default
                                                                                                                    //constructed only if executor is default-constructible, but
                                                                                                                    //io_context::executor_type is not default-constructible
    Logger_srvr_http(boost::asio::io_context& ioc, const char* service, const char* log_dir,
                     std::uintmax_t size = 1000): Logger(service, log_dir, size), _strand(ioc.get_executor()) { }
    void set_strand(boost::asio::io_context& ioc) { _strand = boost::asio::make_strand(ioc.get_executor()); }   //We can assign strand objects, operator= for strand objects is not deprecated
    void make_record(const std::string&) override;
};

#endif // LOGGER_H
