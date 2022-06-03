#include <chrono>
#include <iostream>
#include <iomanip>
#include <system_error>
#include <algorithm>
#include <vector>
#include "logger.h"

Logger::Logger(const std::string& service_, const std::string& log_dir): service(service_)
{
    open_file(log_dir);
    if (f_stream.is_open())
    {
        is_started = true;
        make_record(service + " started");
    }
}

Logger::~Logger()
{
    make_record(service + " stopped");
    f_stream.close();
}

bool Logger::compare(std::filesystem::directory_entry const& first, std::filesystem::directory_entry const& second)
{
    return first.last_write_time() < second.last_write_time();
}

void Logger::open_file(const std::string& log_dir)
{
    namespace fs = std::filesystem;
    fs::path log_folder_path{log_dir};
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
