#include <functional>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <system_error>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>
#include "server_http.h"

namespace server_http
{
b_b::string_view Audio_mime_type::mime_type(b_b::string_view target)
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

bool Find_file::find(b_b::string_view& target)
{
    if (target.size() == 0 || target.back() == '/' || target.find("..") != std::string_view::npos)
    {
        target = "Illegal target filename";
        return false;
    }
    static std::function<bool(b_b::string_view)> compare;
    if (target.back() == '*')
    {
        target.remove_suffix(1);
        compare = [&target] (b_b::string_view curr_file) mutable {
            if (curr_file.starts_with(target))
            {
                target = curr_file;
                return true;
            }
            return false; };
    } else {
        compare = [target] (b_b::string_view curr_file)->bool { return curr_file == target; };
    }
    std::error_code ec;
    bool _is_directory = std::filesystem::is_directory(files_path, ec);
    if (ec)
    {
        std::cerr << "Unable to check if given path to files dir is directory: " << ec.message() << std::endl;
        return false;
    }
    if (_is_directory)
    {
        b_b::string_view curr_file;
        for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
        {
            if (dir_entry.is_regular_file())
            {
                curr_file = dir_entry.path().filename().c_str();
                if (compare(curr_file))
                {
                    std::filesystem::path temp{curr_file.data(), curr_file.data() + curr_file.size()};
                    target = (files_path /= temp).string();
                    return true;
                }
            }
        }
    } else {
        std::cerr << "Given path to files dir is not a directory" << std::endl;
    }
    return false;
}

Srvr_hlpr_clss::Srvr_hlpr_clss(std::shared_ptr<Parser> parser_, bool bool_logger, bool bool_if_fail,
                               bool bool_mime_type, bool bool_find_file, bool bool_handle_request): parser(parser_)
{
    if (bool_logger)
    {
        logger = std::make_shared<Logger>(parser->parsed_info_ptr()[SERVER_NAME],
                                                    parser->parsed_info_ptr()[LOGS_FOLDER],
                                                    std::strtoumax((parser->parsed_info_ptr()[MAX_LOG_FILE_SIZE]), nullptr, 10));
        if (!logger->whether_started())
        {
            logger.reset();
        }
    }
    if (bool_if_fail)
    {
        if_fail = std::make_shared<If_fail>(logger);
    }
    if (bool_mime_type)
    {
        mime_type = std::make_shared<Audio_mime_type>();
    }
    if (bool_find_file)
    {
        find_file = std::make_shared<Find_file>(parser_->parsed_info_ptr()[FILES_FOLDER]);
    }
    if (bool_handle_request)
    {
        handle_request = std::make_shared<Handle_request>(mime_type, find_file, if_fail,
                                                          parser->parsed_info_ptr()[SERVER_NAME]);
    }
}

void If_fail::fail_report(boost::system::error_code ec, const char* reason) const
{
    std::string message = reason + ec.message();
    if (logger)
    {
        logger->make_record(message);
    }
    std::lock_guard<std::mutex> lock(m);
    std::cerr << message << std::endl;
}

Server_HTTP::Server_HTTP(std::shared_ptr<Srvr_hlpr_clss> s_h_c_): s_h_c(s_h_c_),
    num_threads{std::max<int>(1, std::min<int>(std::thread::hardware_concurrency(), std::atoi(s_h_c->parser->parsed_info_ptr()[NUM_THREADS])))},
    ioc{num_threads}
{
    //For now on we have created io_context object, so we may complete our Srvr_hlpr_clss
    auto& parser = s_h_c->parser;
    std::shared_ptr<Logger> logger = std::make_shared<Logger_srvr_http>(ioc, parser->parsed_info_ptr()[SERVER_NAME],
                                                                        parser->parsed_info_ptr()[LOGS_FOLDER],
                                                                        std::strtoumax((parser->parsed_info_ptr()[MAX_LOG_FILE_SIZE]),
                                                                                       nullptr, 10));
    if (!logger->whether_started())
    {
        logger.reset();
    }
    s_h_c->set_logger(logger);
    s_h_c->set_if_fail(std::make_shared<If_fail>(logger));
    s_h_c->set_handle_request(std::make_shared<Handle_request>(s_h_c->mime_type, s_h_c->find_file, s_h_c->if_fail,
                                                               s_h_c->parser->parsed_info_ptr()[SERVER_NAME]));
    if (s_h_c->logger)
    {
        std::string rec = s_h_c->parser->parsed_info_ptr()[SERVER_NAME] + std::string{" starting..."};
        s_h_c->logger->make_record(rec);
    } else {
        std::cerr << "Server_HTTP logger didn't start, continue without logging" << std::endl;
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
    std::make_shared<Listener>(ioc, s_h_c)->run();
    ioc_threads.reserve(num_threads - 1);
    for (auto i = 0; i != (num_threads - 1); ++i)
    {
        ioc_threads.emplace_back(std::thread([&ioc = ioc] () { ioc.run(); }));
    }
    ioc.run();
}

Listener::Listener(b_a::io_context& ioc_, std::shared_ptr<Srvr_hlpr_clss> s_h_c_): ioc(ioc_), s_h_c(s_h_c_),
    _acceptor{b_a::make_strand(ioc)}, _socket{b_a::make_strand(ioc)}
{
    auto parser = s_h_c->parser;
    auto if_fail = s_h_c->if_fail;
    boost::system::error_code ec;
    b_a::ip::address address = b_a::ip::make_address(parser->parsed_info_ptr()[ADDRESS], ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Something wrong with ip address: ");
        return ;
    }
    port_type port = static_cast<port_type>(std::atoi(parser->parsed_info_ptr()[PORT]));
    b_a_i_t::endpoint _endpoint{address, port};
    _acceptor.open(_endpoint.protocol(), ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Failed to open acceptor using specified ip address and port: ");
        return ;
    }
    _acceptor.set_option(b_a::socket_base::reuse_address(true), ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Failed to allow the socket to be bound to an address that is already in use: ");
    }
    //Bind to the server address
    _acceptor.bind(_endpoint, ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Failed to bind to the server address: ");
        return ;
    }
    _acceptor.listen(b_a::socket_base::max_listen_connections, ec);
    if (ec)
    {
        if_fail->fail_report(ec, "Failed to set the maximum length of the queue of pending incoming connections: ");
    }
    s_h_c->logger->make_record(std::string("Server " + std::string(parser->parsed_info_ptr()[SERVER_NAME]) + " starts listening port "
+ std::string(parser->parsed_info_ptr()[PORT]) + " with IP-address: " + std::string(parser->parsed_info_ptr()[ADDRESS])));
}

void Listener::accepting_loop(boost::system::error_code ec)
{
    auto if_fail = s_h_c->if_fail;
#include <boost/asio/yield.hpp>
    reenter(*this)
    {
        for (;;)
        {
            std::cout << "Before async_accept" << std::endl;
            yield _acceptor.async_accept(_socket, b_b::bind_front_handler(&Listener::accepting_loop, shared_from_this()));
            std::cout << "AFTER async_accept" << std::endl;
            if (ec)
            {
                if_fail->fail_report(ec, "Failed to accept: ");
            } else {
                std::make_shared<Session>(std::move(_socket), s_h_c)->run();
            }
            //Make sure each session gets its own strand
            _socket = b_a_i_t::socket(b_a::make_strand(ioc));
        }
    }
#include <boost/asio/unyield.hpp>
}

Session::Session(b_a_i_t::socket&& socket_, std::shared_ptr<Srvr_hlpr_clss> s_h_c_): _stream(std::move(socket_)), s_h_c(s_h_c_)
{
    boost::system::error_code ec;
    auto remote_endpoint = _stream.socket().remote_endpoint(ec);
    if (ec)
    {
        s_h_c->if_fail->fail_report(ec, "Failed to get remote endpoint: ");
    } else {
        auto remote_address = remote_endpoint.address();
        auto remote_port = remote_endpoint.port();
        s_h_c->logger->make_record("Connected to IP-address: " + remote_address.to_string() + ", port: " + std::to_string(remote_port));
    }
}

void Session::run()
{
    //We need to be executing within a strand to perform async operations on the I/O objects in this session
    b_a::dispatch(_stream.get_executor(), b_b::bind_front_handler(&Session::session_loop, shared_from_this(),
                                                                  false, boost::system::error_code{}, 0));
}

void Session::session_loop(bool close, boost::system::error_code ec, size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    auto send_lambda = [this] (auto msg) mutable
    {
        auto res_sp = std::make_shared<decltype(msg)>(std::move(msg));
        this->_res_sp = res_sp;
        b_b_http::async_write(this->_stream, *res_sp, b_b::bind_front_handler(&Session::session_loop, this->shared_from_this(),
                                                                              res_sp->need_eof()));
    };
    static auto if_fail = s_h_c->if_fail;
    static auto handle_request = s_h_c->handle_request;
#include <boost/asio/yield.hpp>
    reenter(*this)
    {
        for(;;)
        {
            //Make the request empty before reading, otherwise the operation behavior is undefined
            req = { };
            //Set the timeout
            _stream.expires_after(std::chrono::minutes(1));
            yield b_b_http::async_read(_stream, _buffer, req, b_b::bind_front_handler(&Session::session_loop, shared_from_this(), false));
            if (ec == b_b_http::error::end_of_stream)
            {
                if_fail->fail_report(ec, "The remote host closed the connection: ");
                break;

            }
            if (ec)
            {
                if_fail->fail_report(ec, "Error while reading message: ");
                break;
            }
            yield handle_request->handle(std::move(req), send_lambda);
            if (ec)
            {
                if_fail->fail_report(ec, "Error while writing response: ");
                break;
            }
            if (close)
            {
                //This means we should close the connection, usually because the response indicated the "Connection: close" semantic
                break;
            }
            //We're done with the response so delete it
            _res_sp = nullptr;
        }
        if (ec != b_b::error::timeout)
        {
            //Send a TCP shutdown
            _stream.socket().shutdown(b_a_i_t::socket::shutdown_send, ec);
        }
    }
#include <boost/asio/unyield.hpp>
}
}   //namespace server_http



