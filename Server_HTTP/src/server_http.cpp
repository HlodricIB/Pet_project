#include <functional>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <boost/system/error_code.hpp>
#include "server_http.h"

namespace server_http
{
std::string_view Audio_mime_type::mime_type(std::string_view target)
{
    size_t pos = target.find_last_of('.');
    if (pos == s_w::npos)
    {
        return "application/octet-stream";
    }
    auto target_ext = target.substr(pos + 1);
    auto found = types.find(target_ext);
    if (found != types.end())
    {
        return found->second;
    }
    return "application/octet-stream";
}

bool Find_file::find(std::string_view& target)
{
    if (target.size() == 0 || target.back() == '/' || target.find("..") != std::string_view::npos)
    {
        target = "Illegal target filename";
        return false;
    }
    static std::function<bool(std::string_view)> compare;
    if (target.back() == '*')
    {
        target.remove_suffix(1);
        compare = [target] (std::string_view curr_file) mutable {
            if (curr_file.starts_with(target))
            {
                target = curr_file;
                return true;
            }
            return false; };
    } else {
        compare = [target] (std::string_view curr_file)->bool { return curr_file == target; };
    }
    std::string_view curr_file;
    for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
    {
        if (dir_entry.is_regular_file())
        {
            curr_file = dir_entry.path().filename().c_str();
            if (compare(curr_file))
            {
                return true;
            }
        }
    }
    target = "Target file not found";
    return false;
}

void If_fail::fail_report(boost::system::error_code ec, const char* reason) const
{
    std::string message = reason + ec.message();
    if (logger)
    {
        logger->make_record(message);
    }
    std::cerr << message << std::endl;
}

Server_HTTP::Server_HTTP(std::shared_ptr<Parser> parser_): parser(parser_),
    num_threads{std::max<int>(1, std::min<int>(std::thread::hardware_concurrency(), std::atoi(parser->parsed_info_ptr()[NUM_THREADS])))},
    ioc{num_threads}
{
    logger = std::make_shared<Logger>(parser->parsed_info_ptr()[SERVER_NAME], parser->parsed_info_ptr()[FILES_FOLDER],
                                      parser->parsed_info_ptr()[MAX_LOG_FILE_SIZE]);
    if (logger->whether_started())
    {
        std::string rec = parser->parsed_info_ptr()[SERVER_NAME] + std::string{" starting..."};
        logger->make_record(rec);
    } else {
        std::cerr << "Server_HTTP logger didn't start, continue without logging" << std::endl;
        logger.reset();
    }
}

Server_HTTP::~Server_HTTP()
{
    ioc.stop();
    for (auto& t : ioc_threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

void Server_HTTP::run()
{
    std::make_shared<Listener>(logger, parser)->run();
    ioc_threads.reserve(num_threads - 1);
    for (auto i = 0; i != (num_threads - 1); ++i)
    {
        ioc_threads.emplace_back(std::thread([&ioc = ioc] () { ioc.run(); }));
    }
    ioc.run();
}

Listener::Listener(b_a::io_context& ioc_, std::shared_ptr<Parser> parser_, std::shared_ptr<Logger> logger_): ioc(ioc_), parser(parser_), logger(logger_),
    _acceptor{b_a::make_strand(ioc)}, _socket{b_a::make_strand(ioc)}
{
    if_fail = std::make_shared<If_fail>(logger);
    boost::system::error_code ec;
    b_a::ip::address address = b_a::ip::make_address(parser->parsed_info_ptr()[ADDRESS], ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Something wrong with ip address: ");
        return ;
    }
    port_type port = static_cast<port_type>(std::atoi(parser->parsed_info_ptr()[1]));
    b_a_i_t::endpoint _endpoint{address, port};
    _acceptor.open(_endpoint.protocol(), ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Failed to open acceptor using specified ip address and port: ");
        return ;
    }
}

Session::Session(std::shared_ptr<Parser> parser_, std::shared_ptr<Logger> logger_): logger(logger_)
{

}

}   //namespace server_http



