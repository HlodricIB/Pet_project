#include <cstdlib>
#include <functional>
#include <chrono>
#include <string>
#include <cctype>
#include <sstream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/dispatch.hpp>
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
void Client_HTTP::Messaging::synch_cout(const std::string_view message)
{
    std::lock_guard<std::mutex> lock(m_cout);
    std::cout << message << std::flush;
}

std::string Client_HTTP::Messaging::synch_cin()
{
    static std::string target;
    std::lock_guard<std::mutex> lock(m_cin);
    std::getline(std::cin, target);
    return target;
}

Client_HTTP::Client_HTTP(std::shared_ptr<Parser> parser):
    num_threads{std::max<int>(1, std::min<int>(std::thread::hardware_concurrency(), std::atoi(parser->parsed_info_ptr()[NUM_THREADS])))},
    ioc{num_threads},
    client_name(parser->parsed_info_ptr()[CLIENT_NAME]),
    msgng()
{
    std::string_view h_a{parser->parsed_info_ptr()[HOST_ADDRESS]};
    std::string_view p_s{parser->parsed_info_ptr()[PORT_SERVICE]};
    parse(h_a, host_address);
    parse(p_s, port_service);
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

void Client_HTTP::parse(const std::string_view& s_w, std::vector<std::string>& c)
{
    std::string_view::size_type pos_begin = 0;
    auto pos_end = pos_begin;
    const auto& _npos = std::string_view::npos;
    while (pos_end != _npos)
    {
        pos_end = s_w.find(",", pos_begin);
        c.push_back(std::string(s_w.substr(pos_begin, (pos_end - pos_begin))));
        //Exclude spaces
        for (pos_begin = pos_end; pos_begin != _npos  && std::isspace(s_w[++pos_begin]); );
        pos_end = pos_begin;
    }
}

void Client_HTTP::run()
{
    auto num_coroutines = std::min(port_service.size(), host_address.size());
    for (std::vector<std::string>::size_type i = 0; i != num_coroutines; ++i)
    {
        b_a::spawn(ioc, std::bind(&Client_HTTP::do_session, this, i, std::placeholders::_1));
    }
    ioc_threads.reserve(num_threads - 1);
    for (int i = 0; i != (num_threads - 1); ++i)
    {
        ioc_threads.emplace_back(std::thread([&ioc = ioc] () { ioc.run(); }));
    }
    ioc.run();
}

void Client_HTTP::do_session(std::vector<std::string>::size_type i, b_a::yield_context yield)
{
    using b_a_i_t = b_a::ip::tcp;
    namespace b_b_http = b_b::http;
    b_a_i_t::resolver resolver(ioc);
    b_b::tcp_stream stream(ioc);
    boost::system::error_code ec;
    //Look up the domain name
    auto const results = resolver.async_resolve(host_address[i], port_service[i], yield[ec]);
    if (ec)
    {
        std::string reason = "Error while resolving host_address: " + host_address[i] + ", port_service: "
+ port_service[i] + ", ";
        return fail_report(ec, reason.c_str());
    }
    //Set the timeout
    stream.expires_after(std::chrono::seconds(30));
    //Make the connection on the IP address we get from a lookup
    stream.async_connect(results, yield[ec]);
    if (ec)
    {
        std::string reason = "Error while connecting to resolved IP address with given host_address: " + host_address[i]
                + ", port_service: " + port_service[i] + ", ";
        return fail_report(ec, reason.c_str());
    }
    std::stringstream message;
    std::string target;
    for (;;)
    {
        //Rewind
        message.seekp(0);
        message << "Enter target for request (\":q\" to close connection): (" << std::this_thread::get_id() << ")\n";
        msgng.synch_cout(message.view());
        target = msgng.synch_cin();
        if (target == ":q")
        {
            break;
        }
        //Set up an HTTP GET request message
        b_b_http::request<b_b_http::string_body> req{b_b_http::verb::get, target, version};
        req.set(b_b_http::field::host, host_address[i]);
        req.set(b_b_http::field::user_agent, client_name);
        //Set the timeout
        stream.expires_after(std::chrono::seconds(30));
        //Send the HTTP request to the remote host
        b_b_http::async_write(stream, req, yield[ec]);
        if (ec)
        {
            std::string reason = "Error while writing to resolved IP address with given host_address: " + host_address[i]
                    + ", port_service: " + port_service[i] + ", ";
            fail_report(ec, reason.c_str());
            break;
        }
        //This buffer is used for reading and must be persisted
        b_b::flat_buffer buffer;
        //Declare a container to hold the response
        b_b_http::response<b_b_http::dynamic_body> res;
        b_b_http::async_read(stream, buffer, res, yield[ec]);
        if (ec)
        {
            std::string reason = "Error while receiving response from resolved IP address with given host_address: "
+ host_address[i] + ", port_service: " + port_service[i] + ", ";
            fail_report(ec, reason.c_str());
            break;
        }
        //Write the message to standard out
        std::cout << res << std::endl;
    }
    if (ec != b_b::error::timeout)
    {
        std::cout << "Shutting down (" << std::this_thread::get_id() << ")\n";
        stream.socket().shutdown(b_a_i_t::socket::shutdown_both, ec);
        //Not_connected happens sometimes so don't bother reporting it
        if (ec && (ec != b_b::errc::not_connected))
        {
            std::string reason = "Error while closing connection to resolved IP address with given host_address: "
+ host_address[i] + ", port_service: " + port_service[i] + ", ";
            fail_report(ec, reason.c_str());
        }
    }
    //If we get here then the connection is closed gracefully
}
}   //namespace client_http
