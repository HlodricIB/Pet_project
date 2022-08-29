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

#include "parser.h"
#include "logger.h"

//#include <chrono>
//#include <iostream>
//#include <boost/asio/steady_timer.hpp>
//#include <boost/asio/coroutine.hpp>
//#include <boost/beast/core/bind_handler.hpp>

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

class If_fail
{
private:
    std::shared_ptr<Logger> logger{nullptr};
public:
    If_fail(std::shared_ptr<Logger> logger_): logger(logger_) { }
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
    std::shared_ptr<Parser> parser{0};
    int num_threads{0};
    b_a::io_context ioc;
    std::shared_ptr<Logger> logger{0};
    std::vector<std::thread> ioc_threads;   //Threads for ioc to run
    void run();
public:
    Server_HTTP(std::shared_ptr<Parser>);
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

class Listener
{
private:
    b_a::io_context& ioc;
    std::shared_ptr<Parser> parser{nullptr};
    std::shared_ptr<Logger> logger{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    b_a_i_t::acceptor _acceptor;
    b_a_i_t::socket _socket;
public:
    Listener(b_a::io_context&, std::shared_ptr<Parser>, std::shared_ptr<Logger>);
    void run();
};

class Session
{
private:
    std::shared_ptr<Logger> logger{nullptr};
    std::shared_ptr<If_fail> if_fail{nullptr};
    std::shared_ptr<Mime_types> mime_type{nullptr};
    std::shared_ptr<Find_file> find_file{nullptr};
    std::string_view server_name;
    template<class Body, class Allocator, class Sender>
    void
    handle_request(std::string_view filepath_base, b_b_http::request<Body, b_b_http::basic_fields<Allocator>>&& req, Sender&& sender)
    {
        boost::ignore_unused(filepath_base);
        boost::ignore_unused(req);
        boost::ignore_unused(sender);
    }
public:
    Session(std::shared_ptr<Parser>, std::shared_ptr<Logger>);

};
}   //namespace server_http


#endif // SERVER_HTTP_H
