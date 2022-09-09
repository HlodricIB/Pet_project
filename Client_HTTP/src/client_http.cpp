#include <cstdlib>
#include <functional>
#include <chrono>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include "client_http.h"

namespace client_http
{
Client_HTTP::Client_HTTP(std::shared_ptr<Parser> parser):
    num_threads{std::max<int>(1, std::min<int>(std::thread::hardware_concurrency(), std::atoi(parser->parsed_info_ptr()[NUM_THREADS])))},
    ioc{num_threads},
    host_address(parser->parsed_info_ptr()[HOST_ADDRESS]), port_service(parser->parsed_info_ptr()[PORT_SERVICE]),
    client_name(parser->parsed_info_ptr()[CLIENT_NAME])
{
    const char* _version = parser->parsed_info_ptr()[VERSION];
    version = (std::strcmp(_version, "\0") || !std::strcmp(_version, "1.0")) ? 10 : 11;
}

Client_HTTP::~Client_HTTP()
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

void Client_HTTP::run()
{
    b_a::spawn(ioc, std::bind(&Client_HTTP::do_session, shared_from_this(), std::placeholders::_1));
    ioc_threads.reserve(num_threads - 1);
    for (auto i = 0; i != (num_threads - 1); ++i)
    {
        ioc_threads.emplace_back(std::thread([&ioc = ioc] () { ioc.run(); }));
    }
    ioc.run();
}

void Client_HTTP::do_session(b_a::yield_context yield)
{
    using b_a_i_t = b_a::ip::tcp;
    namespace b_b_http = b_b::http;
    b_a_i_t::resolver resolver(ioc);
    b_b::tcp_stream stream(ioc);
    boost::system::error_code ec;
    //Look up the domain name
    auto const results = resolver.async_resolve(host_address, port_service, yield[ec]);
    if (ec)
    {
        fail_report(ec, "Error while resolving host_address, port_service: ");
    }
    //Set the timeout
    stream.expires_after(std::chrono::seconds(30));
    //Make the connection on the IP address we get from a lookup
    stream.async_connect(results, yield[ec]);
    if (ec)
    {
        fail_report(ec, "Error while connecting to resolved IP address: ");
    }
    std::string target;
    for (;;)
    {
        std::cout << "Enter target fo request (\":q\" to close connection):\n";
        std::getline(std::cin, target);
        if (target == ":q")
        {
            break;
        }
        //Set up an HTTP GET request message
        b_b_http::request<b_b_http::string_body> req{b_b_http::verb::get, target, version};
        req.set(b_b_http::field::host, host_address);
        req.set(b_b_http::field::user_agent, client_name);
        //Set the timeout
        stream.expires_after(std::chrono::seconds(30));
        //Send the HTTP request to the remote host
        b_b_http::async_write(stream, req, yield[ec]);
        if (ec)
        {
            fail_report(ec, "Error while sending request: ");
        }
        //This buffer is used for reading and must be persisted
        b_b::flat_buffer buffer;
        //Declare a container to hold the response
        b_b_http::response<b_b_http::dynamic_body> res;
        b_b_http::async_read(stream, buffer, res, yield[ec]);
        if (ec)
        {
            fail_report(ec, "Error while receiving response: ");
        }
        //Write the message to standard out
        std::cout << res << std::endl;
    }
    stream.socket().shutdown(b_a_i_t::socket::shutdown_both, ec);
    if (ec && (ec != b_b::errc::not_connected))
    {
        fail_report(ec, "Error while closing connection: ");
    }
    //If we get here then the connection is closed gracefully
}
}   //namespace client_http
