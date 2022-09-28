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
    NUM_THREADS
};

namespace b_a = boost::asio;
namespace b_b = boost::beast;

class Client_HTTP
{
private:
    struct Messaging
    {
        std::mutex m_cin;
        std::mutex m_cout;
        Messaging() { }
        void synch_cout(const std::string_view);
        std::string synch_cin();
    };
    int num_threads{0};
    b_a::io_context ioc;
    b_b::string_view client_name;
    Messaging msgng;
    std::vector<std::string> host_address, port_service;
    int version;
    std::vector<std::thread> ioc_threads;   //Threads for ioc to run
    void parse(const std::string_view&, std::vector<std::string>&);
    void fail_report(boost::system::error_code ec, const char* reason) const { std::cerr << reason << ec.message() << std::endl; }
    void do_session(std::vector<std::string>::size_type, b_a::yield_context);
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
