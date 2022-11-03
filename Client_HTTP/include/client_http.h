#ifndef CLIENT_HTTP_H
#define CLIENT_HTTP_H

#define BOOST_ALLOW_DEPRECATED_HEADERS  //To silence warning that <boost/detail/scoped_enum_emulation.hpp> is deprecated

#include <thread>
#include <memory>
#include <vector>
#include <iostream>
#include <string_view>
#include <mutex>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include "parser.h"

namespace client_http
{
//An order of char* returned by Parser
enum
{
    HOST_ADDRESS,
    PORT_SERVICE,
    CLIENT_NAME,
    VERSION,
    NUM_THREADS,
    DOWNLOAD_DIR
};

namespace b_a = boost::asio;
namespace b_b = boost::beast;
namespace b_b_http = b_b::http;

class Client_HTTP
{
private:
    struct Messaging
    {
    private:
        std::mutex m;
    public:
        Messaging() { }
        template<typename Message>
        void
        synch_cout(const Message message)
        {
            std::lock_guard<std::mutex> lock(m);
            std::cout << message << std::flush;
        }
        template<typename Message>
        std::string
        synch_cin(const Message message, bool if_cout)
        {
            static thread_local std::string target;
            std::lock_guard<std::mutex> lock(m);
            {
                if (if_cout)
                {
                    std::cout << message << std::flush;
                }
                std::getline(std::cin, target);
            }
            return target;
        }
    };
    int num_threads{0};
    b_a::io_context ioc;
    b_b::string_view client_name;
    Messaging msgng;
    std::vector<std::string> host_address, port_service, download_dirs;
    int version;
    std::vector<std::thread> ioc_threads;   //Threads for ioc to run
    void parse(const std::string_view&, std::vector<std::string>&);
    void fail_report(boost::system::error_code ec, const char* reason) const { std::cerr << reason << ec.message() << std::endl; }
    void do_session(std::vector<std::string>::size_type, b_a::yield_context);
    void set_params_file_body_parser(b_b_http::response_parser<b_b_http::file_body>&, const std::filesystem::path&); //Second argument is a reference to download dir the appropriate
                                                                                                                     //directory for the corresponding host and port
public:
    explicit Client_HTTP(std::shared_ptr<Parser>);
    Client_HTTP(const Client_HTTP&) = delete;
    Client_HTTP(Client_HTTP&) = delete;
    Client_HTTP& operator=(const Client_HTTP&) = delete;
    Client_HTTP& operator=(Client_HTTP&) = delete;
    ~Client_HTTP();
    void run();
};
}   //namespace client_http

#endif // CLIENT_HTTP_H
