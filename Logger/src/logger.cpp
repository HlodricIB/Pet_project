#include <chrono>
#include <iostream>
#include <iomanip>
#include <system_error>
#include <algorithm>
#include <vector>
#include "logger.h"

Logger::Logger(const std::string& service_, const std::string& log_dir, std::uintmax_t size): service(service_), max_log_file_size(size)
{
    open_file(log_dir);
    if (f_stream.is_open())
    {
        is_started = true;
    }
}

Logger::~Logger()
{
    f_stream.close();
}

bool Logger::compare(std::filesystem::directory_entry const& first, std::filesystem::directory_entry const& second)
{
    return first.last_write_time() < second.last_write_time();
}

bool Logger::create_dir_if_not_exist(const std::filesystem::path & log_folder_path)
{
    std::cout << "Specified directory for log files doesn't exist, create ";
    char c;
    std::error_code e_c;
    while(true)
    {
        std::cout << "[y/n]?\n";
        if (!(std::cin >> c))
        {
            std::cin.clear();
            continue;
        }
        std::cin.get();
        switch (c)
        {
        case 'y' :
            if (!std::filesystem::create_directories(log_folder_path, e_c))
            {
                std::cerr << "Unable to create specified directory for log files: " << e_c.message() << std::endl;
                return false;
            }
            return true;
            break;
        case 'n' :
            return false;
            break;
        default:
            if (std::cin.rdstate() == std::ios_base::badbit)
            {
                return false;
            } else
            {
                continue;
            }
        }
    }
}

void Logger::open_file(const std::string& log_dir)
{
    namespace fs = std::filesystem;
    fs::path log_folder_path{log_dir};
    if (!fs::exists(log_folder_path))
    {
        if (!create_dir_if_not_exist(log_folder_path))
        {
            return;
        }
    }
    auto begin = fs::directory_iterator(log_folder_path);
    auto end = fs::directory_iterator();
    if (begin != end)
    {
        std::vector<fs::directory_entry> entries;
        do
        {
            if (begin->is_regular_file())
            {
                entries.push_back(*begin);
                ++n;
            }

        } while (++begin != end);
        auto last_modified_file = std::max_element(entries.begin(), entries.end(), compare);    //Searching for last time write file
        std::error_code ec;
        if ((last_modified_file->file_size(ec) <= max_log_file_size) && (!ec))  //If last time write file size is smaller than max_log_file_size, then open this file, otherwise create new file
        {
            log_file = last_modified_file->path();
            f_stream = std::fstream(log_file, std::ios::app);
            return;
        }
    }
    fs::path new_log_file{log_folder_path};
    new_log_file /= std::string(service + "_log" + std::to_string(n) + ".log");
    f_stream = std::fstream(new_log_file, std::ios::out);
}

void Logger::make_record(const std::string& event)
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    f_stream << std::put_time(std::localtime(&now_time_t), "%c \t") << event << "\n";
}
