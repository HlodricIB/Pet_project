#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include <string_view>
#include <string>
#include <memory>
#include <map>
#include <filesystem>
#include <thread>
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
#include "parser.h"
#include "logger.h"

//#include <chrono>
//#include <iostream>
//#include <boost/asio/steady_timer.hpp>

namespace server_http
{

class Mime_types
{
public:
    virtual ~Mime_types() { }
    virtual std::string_view mime_type(std::string_view) = 0;
};

class Audio_mime_type : public Mime_types
{
private:
    using s_w = std::string_view;
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
    std::string_view mime_type(std::string_view) override;
};

class Find_file
{
private:
    std::filesystem::path files_path;
public:
    explicit Find_file(const char* files_path_): files_path(files_path_) { };
    bool find(std::string_view&);
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
    Srvr_hlpr_clss(std::shared_ptr<Parser>);
};

class If_fail
{
private:
    std::shared_ptr<Logger> logger{nullptr};
public:
    If_fail(std::shared_ptr<Srvr_hlpr_clss> s_h_c): logger(s_h_c->logger) { }
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
    void run();
public:
    Server_HTTP(std::shared_ptr<Srvr_hlpr_clss>);
    Server_HTTP(const Server_HTTP&) = delete;
    Server_HTTP(Server_HTTP&) = delete;
    Server_HTTP& operator=(const Server_HTTP&) = delete;
    Server_HTTP& operator=(Server_HTTP&) = delete;
    ~Server_HTTP();
};

namespace b_b = boost::beast;
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
    void run() { accepting_loop(); };
};

class Handle_request
{
private:
    std::shared_ptr<Mime_types> mime_type{nullptr};
    std::shared_ptr<Find_file> find_file{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    std::string_view server_name;
    template<class Body, class Allocator, class Sender>
    void
    handle(std::string_view filepath_base, b_b_http::request<Body, b_b_http::basic_fields<Allocator>>&& req, Sender&& sender)
    {
        boost::ignore_unused(filepath_base);
        boost::ignore_unused(req);
        boost::ignore_unused(sender);
    }
public:
    Handle_request(std::shared_ptr<Mime_types> mime_type_, std::shared_ptr<Find_file> find_file_, std::shared_ptr<If_fail> if_fail_, std::string_view server_name_):
        mime_type(mime_type_), find_file(find_file_), if_fail(if_fail_), server_name(server_name_) { }
};

class Session : public b_a::coroutine, public std::enable_shared_from_this<Session>
{
private:
    b_b::tcp_stream _stream;
    std::shared_ptr<Srvr_hlpr_clss> s_h_c{nullptr};
    b_b_http::request<b_b_http::string_body> req;
    b_b::flat_buffer _buffer;
    std::shared_ptr<void> _res_sp{nullptr};    //For managing the message to extent it's lifetime for the duration of the async operation
    void run();
    void session_loop(bool, boost::system::error_code, size_t);

public:
    Session(b_a_i_t::socket&& socket, std::shared_ptr<Srvr_hlpr_clss>);

};
}   //namespace server_http


#endif // SERVER_HTTP_H
