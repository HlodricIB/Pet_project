#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include <string_view>
#include <string>
#include <memory>
#include <map>
#include <filesystem>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>
#include <sstream>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include "parser.h"
#include "logger.h"
#include "handler.h"


#include <iostream>

namespace server_http
{
namespace b_b = boost::beast;

class Mime_types
{
public:
    virtual ~Mime_types() { }
    virtual b_b::string_view mime_type(b_b::string_view) = 0;
};

class Audio_mime_type : public Mime_types
{
private:
    using s_w = b_b::string_view;
    const std::map<s_w, s_w> types{
        {"aac", "audio/aac"},
        {"mid", "audio/midi audio/x-midi"},
        {"midi", "audio/midi audio/x-midi"},
        {"mp3", "audio/mpeg"},
        {"oga", "audio/ogg"},
        {"wav", "audio/wav"},
        {"weba", "audio/webm"},
        {"3gp", "audio/3gpp"},
        {"3g2", "audio/3gpp2"}};
public:
    Audio_mime_type() { }
    b_b::string_view mime_type(b_b::string_view) override;
};

//Forward declarations for Srvr_hlpr_clss
class If_fail;

class Srvr_hlpr_clss
{
    friend class Server_HTTP;
    friend class Listener;
    friend class Session;
    friend class If_fail;
private:
    std::shared_ptr<Parser> parser{nullptr};
    std::shared_ptr<Logger> logger{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    std::shared_ptr<Mime_types> mime_type{nullptr};
    std::shared_ptr<Handler> handler{nullptr};
public:
    Srvr_hlpr_clss() { }
    explicit Srvr_hlpr_clss(std::shared_ptr<Parser>, bool, bool, bool, bool);   //bool arguments are telling
                                                                    //wether appropriate class have to be constructed or not
    void set_parser(std::shared_ptr<Parser> shrd_ptr) { parser = shrd_ptr; }
    void set_logger(std::shared_ptr<Logger> shrd_ptr) { logger = shrd_ptr; }
    void set_if_fail(std::shared_ptr<If_fail> shrd_ptr) { if_fail = shrd_ptr; }
    void set_mime_type(std::shared_ptr<Mime_types> shrd_ptr) { mime_type = shrd_ptr; }
    void set_handler(std::shared_ptr<Handler> shrd_ptr) { handler = shrd_ptr; }
    std::shared_ptr<Parser> get_parser() { return parser; }
    std::shared_ptr<Logger> get_logger() { return logger; }
    std::shared_ptr<If_fail> get_if_fail() { return if_fail; }
    std::shared_ptr<Mime_types> get_mime_type() { return mime_type; }
    std::shared_ptr<Handler> get_handler() { return handler; }

};

class If_fail
{
private:
    std::shared_ptr<Logger> logger{nullptr};
    mutable std::mutex m;
public:
    If_fail() { }
    explicit If_fail(std::shared_ptr<Srvr_hlpr_clss> s_h_c): logger(s_h_c->logger) { }
    explicit If_fail(std::shared_ptr<Logger> logger_ = nullptr): logger(logger_) { }
    void fail_report(boost::system::error_code, const char*) const;
};

//An order of char* returned by Parser_Server_HTTP
enum : size_t
{
    ADDRESS,
    PORT,
    SERVER_NAME,
    FILES_FOLDER,
    LOGS_FOLDER,
    MAX_LOG_FILE_SIZE,
    NUM_THREADS
};

namespace b_a = boost::asio;

class Server_HTTP
{
private:
    std::shared_ptr<Srvr_hlpr_clss> s_h_c{nullptr};
    int num_threads{0};
    b_a::io_context ioc;
    std::vector<std::thread> ioc_threads;   //Threads for ioc to run
public:
    explicit Server_HTTP(std::shared_ptr<Srvr_hlpr_clss>);
    Server_HTTP(const Server_HTTP&) = delete;
    Server_HTTP(Server_HTTP&) = delete;
    Server_HTTP& operator=(const Server_HTTP&) = delete;
    Server_HTTP& operator=(Server_HTTP&) = delete;
    ~Server_HTTP();
    void run();
};

namespace b_b_http = b_b::http;
using b_a_i_t = b_a::ip::tcp;
//As declared in latest versions of boost/asio/ip/basic_endpoint.hpp
typedef uint_least16_t port_type;

class Listener : public b_a::coroutine, public std::enable_shared_from_this<Listener>
{
private:
    b_a::io_context& ioc;
    std::shared_ptr<Srvr_hlpr_clss> s_h_c{nullptr};
    b_a_i_t::acceptor _acceptor;
    b_a_i_t::socket _socket;
    void accepting_loop(boost::system::error_code = { });
public:
    Listener(b_a::io_context&, std::shared_ptr<Srvr_hlpr_clss>);
    Listener(const Listener&) = delete;
    Listener(Listener&) = delete;
    Listener& operator=(const Listener&) = delete;
    Listener& operator=(Listener&) = delete;
    void run() { accepting_loop(); };
};

//An order of parameters in req_info
enum
{
    REQ_TARGET,
    REQ_HOST,
    REQ_PORT,
    REQ_IP_ADDRESS,
    REQ_USER_AGENT,
    REQ_METHOD,
    REQ_RESULT,
    REQ_ERROR
};

class Handle_request
{
private:
    std::shared_ptr<Mime_types> mime_type{nullptr};
    std::shared_ptr<Handler> handler{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    b_b::string_view server_name;
    b_a::ip::address remote_address;
    port_type remote_port;
    std::vector<std::string> req_info;  //Vector to pass to Handler object, this vector shall hold the result of handling
                                        //First element is requested target, second - host, third - port, fourth - ip
                                        //fifth - user_agent, sixth - method, seventh - is for storing result of handling by Handler,
                                        //eighth - is for stroring possible error message
    template<class Body, class Allocator>
    void
    handler_vector_fill(b_b_http::request<Body, b_b_http::basic_fields<Allocator>>& req)    //Method for filling req_info                                                                                      //to fill std::vector<std::string> for handler
    {
        //First element of target_body_info is requested target, second - host, third - port, fourth - ip, fifth - method,
        //sixth - is for storing result of handling by Handler, seventh - is for storing possible error message
        req_info[REQ_TARGET].resize(0);
        req_info[REQ_HOST].resize(0);
        req_info[REQ_USER_AGENT].resize(0);
        req_info[REQ_METHOD].resize(0);
        req_info[REQ_RESULT].resize(0);
        req_info[REQ_ERROR].resize(0);
        auto target = req.target();
        target.remove_prefix(1);
        req_info[REQ_TARGET] = std::string(target.data(), target.size()); //Store target from request
        auto host = req[b_b_http::field::host];
        req_info[REQ_HOST] = std::string(host.data(), host.size());
        auto user_agent = req[b_b_http::field::user_agent];
        req_info[REQ_USER_AGENT] = std::string(user_agent.data(), user_agent.size());
        auto req_method = req.method_string();
        req_info[REQ_METHOD] = std::string(req_method.data(), req_method.size());
    }
public:
    Handle_request(std::shared_ptr<Mime_types> mime_type_, std::shared_ptr<Handler> handler_, std::shared_ptr<If_fail> if_fail_,
                   b_b::string_view server_name_, b_a::ip::address remote_address_, port_type remote_port_):
        mime_type(mime_type_), handler(handler_), if_fail(if_fail_), server_name(server_name_), remote_address(remote_address_), remote_port(remote_port_)
    {
        req_info.reserve(REQ_ERROR + 1);
        req_info.emplace_back("");
        req_info.emplace_back("");
        req_info.emplace_back(std::to_string(remote_port));
        req_info.emplace_back(remote_address.to_string());
        for (size_t i = REQ_IP_ADDRESS; i != REQ_ERROR; ++i)
        {
            req_info.emplace_back("");
        }
    }
    template<class Body, class Allocator, class Sender>
    void
    handle(b_b_http::request<Body, b_b_http::basic_fields<Allocator>>&& req, Sender& sender)
    {
        //Returns this response if type of response body is string_body (http::basic_string_body::value_type = std::basic_string< CharT, Traits, Allocator>)
        auto const string_body_res = [&req, &server_name = server_name] (b_b_http::status status, std::string_view body_content)
        {
            b_b_http::response<b_b_http::string_body> res{status, req.version()};
            res.set(b_b_http::field::server, server_name);
            res.set(b_b_http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = body_content;
            res.prepare_payload();
            return res;
        };
        auto method = req.method();
        //Make sure we can handle the method
        if (method != b_b_http::verb::get && method != b_b_http::verb::head)
        {
            return sender(string_body_res(b_b_http::status::bad_request, "Unknown HTTP method"));
        }
        handler_vector_fill(req);
        auto if_file = handler->handle(req_info);   //Returning bool value shows if we have request for file (true) or
                                                    //request for something else (false)
        //Check if we have a request for files list
        if (!if_file)
        {
            std::string_view songs_list_body{req_info[REQ_RESULT]};
            auto body_size = songs_list_body.size();
            switch (method)
            {
            case b_b_http::verb::head:
            {
                b_b_http::response<b_b_http::empty_body> res{b_b_http::status::ok, req.version()};
                res.set(b_b_http::field::server, server_name);
                res.set(b_b_http::field::content_type, "text/plain");
                res.content_length(body_size);
                res.keep_alive(req.keep_alive());
                return sender(std::move(res));
                break;
            }
            case b_b_http::verb::get:
            {
                return sender(string_body_res(b_b_http::status::ok, songs_list_body));
                break;
            }
            default:
            {
                std::string fail_message{"Unknown HTTP method in request from ip-address: " + remote_address.to_string()
                                            + ", port: " + std::to_string(remote_port) + '\n'};
                if_fail->fail_report({ }, fail_message.c_str());
                return sender(string_body_res(b_b_http::status::bad_request, "Unknown HTTP method"));
                break;
            }
            }
        } else {
            //Now we can be sure, that we have a request for file
            if (req_info[REQ_RESULT].empty())
            {
                // Something wrong, sending back appropriate response
                auto& what_is_wrong = req_info[REQ_ERROR];
                std::string fail_message{what_is_wrong + " (request from ip-address: " + remote_address.to_string()
                                                                            + ", port: " + std::to_string(remote_port) + ")"};
                if_fail->fail_report({ }, fail_message.c_str());
                what_is_wrong.push_back('\n');
                if (what_is_wrong.starts_with("Illegal request"))
                {
                    //If we are here, then we have illegal request-target
                    return sender(string_body_res(b_b_http::status::bad_request, what_is_wrong));
                } else {
                    if (what_is_wrong.starts_with("Several"))
                    {
                        //If we are here, then several files match the request-target at once
                        return sender(string_body_res(b_b_http::status::multiple_choices, what_is_wrong));
                    } else {
                        if (what_is_wrong.starts_with("Database"))
                        {
                            //If we are here, then we have some error with query to database
                            return sender(string_body_res(b_b_http::status::internal_server_error, what_is_wrong));
                        } else {
                            //If we are here, then requested file not found
                            return sender(string_body_res(b_b_http::status::not_found, what_is_wrong));
                        }
                    }
                }
            }
            //Attempt to open the file
            boost::system::error_code ec;
            b_b_http::file_body::value_type body;
            body.open(req_info[REQ_RESULT].c_str(), b_b::file_mode::scan, ec);
            //Handle an unknown error
            if (ec)
            {
                return sender(string_body_res(b_b_http::status::internal_server_error, ec.message()));
            }
            //Store body size since we need it after the move
            auto const size = body.size();
            //Store full path to target file into b_b::string_view for mime_type determining and other purposes
            b_b::string_view target = req_info[REQ_TARGET];
            //Lambda to fill response on request for file
            auto res_fill = [this, &size, &req, &target] (auto& res) {
                std::stringstream c_d_field;
                c_d_field << "attachment; filename=\"" << target << "\"";
                res.set(b_b_http::field::server, server_name);
                res.set(b_b_http::field::content_type, mime_type->mime_type(target));
                res.set(b_b_http::field::content_disposition, c_d_field.str());
                res.content_length(size);
                res.keep_alive(req.keep_alive());
            };
            switch (method)
            {
            case b_b_http::verb::head:
            {
                b_b_http::response<b_b_http::empty_body> res{b_b_http::status::ok, req.version()};
                res_fill(res);
                return sender(std::move(res));
                break;
            }
            case b_b_http::verb::get:
            {
                b_b_http::response<b_b_http::file_body> res{std::piecewise_construct,
                            std::make_tuple(std::move(body)), std::make_tuple(b_b_http::status::ok, req.version())};
                res_fill(res);
                return sender(std::move(res));
                break;
            }
            default:
            {
                return sender(string_body_res(b_b_http::status::bad_request, "Unknown HTTP method"));
                break;
            }
            }
        }
    }
    void set_handler(std::shared_ptr<Handler> handler_) { handler = handler_; } //Method for changing handler
};

class Session : public b_a::coroutine, public std::enable_shared_from_this<Session>
{
private:
    b_b::tcp_stream _stream;
    std::shared_ptr<Srvr_hlpr_clss> s_h_c{nullptr};
    b_b_http::request<b_b_http::string_body> req;
    b_b::flat_buffer _buffer;
    std::shared_ptr<void> _res_sp{nullptr};    //For managing the message to extent it's lifetime for the duration of the async operation
    b_a::ip::address remote_address;
    port_type remote_port;
    void session_loop(bool, boost::system::error_code, size_t);
public:
    Session(b_a_i_t::socket&&, std::shared_ptr<Srvr_hlpr_clss>);
    Session(const Session&) = delete;
    Session(Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&) = delete;
    void run();
};
}   //namespace server_http

#endif // SERVER_HTTP_H
