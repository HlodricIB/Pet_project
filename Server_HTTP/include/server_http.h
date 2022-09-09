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

class Find_file
{
private:
    std::filesystem::path files_path;
    std::string_view filename;
public:
    explicit Find_file(const char* files_path_): files_path(files_path_) { };
    bool find(b_b::string_view&);
    std::string full_path() { return std::string(files_path /= filename); }
};

//Forward declarations for Srvr_hlpr_clss
class If_fail;
class Handle_request;

class Srvr_hlpr_clss
{
    friend class Server_HTTP;
    friend class Listener;
    friend class Session;
    friend class If_fail;
    friend class Handle_request;
private:
    std::shared_ptr<Parser> parser{nullptr};
    std::shared_ptr<Logger> logger{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    std::shared_ptr<Mime_types> mime_type{nullptr};
    std::shared_ptr<Find_file> find_file{nullptr};
    std::shared_ptr<Handle_request> handle_request{nullptr};
public:
    explicit Srvr_hlpr_clss(std::shared_ptr<Parser>);
};

class If_fail
{
private:
    std::shared_ptr<Logger> logger{nullptr};
public:
    If_fail() { }
    explicit If_fail(std::shared_ptr<Srvr_hlpr_clss> s_h_c): logger(s_h_c->logger) { }
    explicit If_fail(std::shared_ptr<Logger> logger_ = nullptr): logger(logger_) { }
    void fail_report(boost::system::error_code, const char*) const;
};

//An order of char* returned by Parser
enum
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

class Handle_request
{
private:
    std::shared_ptr<Mime_types> mime_type{nullptr};
    std::shared_ptr<Find_file> find_file{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    b_b::string_view server_name;
public:
    Handle_request(std::shared_ptr<Mime_types> mime_type_, std::shared_ptr<Find_file> find_file_, std::shared_ptr<If_fail> if_fail_, b_b::string_view server_name_):
        mime_type(mime_type_), find_file(find_file_), if_fail(if_fail_), server_name(server_name_) { }
    template<class Body, class Allocator, class Sender>
    void
    handle(b_b_http::request<Body, b_b_http::basic_fields<Allocator>>&& req, Sender& sender)
    {
        //Returns this response if type of response body is string_body (http::basic_string_body::value_type = std::basic_string< CharT, Traits, Allocator>)
        auto const string_body_res = [&req, &server_name = server_name] (b_b_http::status status, std::string_view body_content)
        {
            b_b_http::response<b_b_http::string_body> res{status, req.version()};
            res.set(b_b_http::field::server, server_name);
            res.set(b_b_http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = body_content;
            res.prepare_payload();
            return res;
        };
        auto target = req.target();
        //Make sure we can handle the method
        if (req.method() != b_b_http::verb::get || req.method() != b_b_http::verb::head)
        {
            return sender(string_body_res(b_b_http::status::bad_request, "Unknown HTTP method"));
        }
        //Check if we have a request for songs list
        if (target.empty() || (target.size() == 1 && target[0] == '/'))
        {
            std::string_view songs_list_body{"Here have to be a songs list"};
            auto body_size = songs_list_body.size();
            switch (req.method())
            {
            case b_b_http::verb::head:
            {
                b_b_http::response<b_b_http::empty_body> res{b_b_http::status::ok, req.version()};
                res.set(b_b_http::field::server, server_name);
                res.set(b_b_http::field::content_type, "text/html");
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
                return sender(string_body_res(b_b_http::status::bad_request, "Unknown HTTP method"));
                break;
            }
            }
        }
        //Now we can be sure, that we have a request fo song, so make sure that requested filename is legal
        if (target[0] == '/' || target.find("..") != b_b::string_view::npos)
        {
            return sender(string_body_res(b_b_http::status::bad_request, "Illegal request-target"));
        }
        //Handle the case where the file doesn't exist
        if (!find_file->find(target))
        {
            std::string body_content = "File " + std::string(target.data(), target.size()) + " not found";
            return sender(string_body_res(b_b_http::status::not_found, body_content));
        }
        //Attempt to open the file
        boost::system::error_code ec;
        b_b_http::file_body::value_type body;
        auto full_path = find_file->full_path();
        body.open(full_path.c_str(), b_b::file_mode::scan, ec);
        //Handle an unknown error
        if (ec)
        {
            return sender(string_body_res(b_b_http::status::internal_server_error, ec.message()));
        }
        auto const size = body.size();
        switch (req.method())
        {
        case b_b_http::verb::head:
        {
            b_b_http::response<b_b_http::empty_body> res{b_b_http::status::ok, req.version()};
            res.set(b_b_http::field::server, server_name);
            res.set(b_b_http::field::content_type, mime_type->mime_type(target));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return sender(std::move(res));
            break;
        }
        case b_b_http::verb::get:
        {
            b_b_http::response<b_b_http::file_body> res{std::piecewise_construct,
                        std::make_tuple(std::move(body)), std::make_tuple(b_b_http::status::ok, req.version())};
            res.set(b_b_http::field::server, server_name);
            res.set(b_b_http::field::content_type, mime_type->mime_type(target));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            break;
        }
        default:
        {
            return sender(string_body_res(b_b_http::status::bad_request, "Unknown HTTP method"));
            break;
        }
        }
    }
};

class Session : public b_a::coroutine, public std::enable_shared_from_this<Session>
{
private:
    b_b::tcp_stream _stream;
    std::shared_ptr<Srvr_hlpr_clss> s_h_c{nullptr};
    b_b_http::request<b_b_http::string_body> req;
    b_b::flat_buffer _buffer;
    std::shared_ptr<void> _res_sp{nullptr};    //For managing the message to extent it's lifetime for the duration of the async operation
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
