#include <cstdlib>
#include <functional>
#include <chrono>
#include <string>
#include <cctype>
#include <sstream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include "client_http.h"

namespace client_http
{
Client_HTTP::Client_HTTP(std::shared_ptr<Parser> parser):
    num_threads{std::max<int>(1, std::min<int>(std::thread::hardware_concurrency(), std::atoi(parser->parsed_info_ptr()[NUM_THREADS])))},
    ioc{num_threads},
    client_name(parser->parsed_info_ptr()[CLIENT_NAME]),
    msgng()
{
    std::string_view h_a{parser->parsed_info_ptr()[HOST_ADDRESS]};
    std::string_view p_s{parser->parsed_info_ptr()[PORT_SERVICE]};
    std::string_view d_d{parser->parsed_info_ptr()[DOWNLOAD_DIR]};
    parse(h_a, host_address);
    parse(p_s, port_service);
    parse(d_d, download_dirs);
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
    stream.expires_after(std::chrono::minutes(1));
    //Make the connection on the IP address we get from a lookup
    stream.async_connect(results, yield[ec]);
    if (ec)
    {
        std::string reason = "Error while connecting to resolved IP address with given host_address: " + host_address[i]
                + ", port_service: " + port_service[i] + ", ";
        return fail_report(ec, reason.c_str());
    }
    std::stringstream message;
    std::string target(1, '/');
    // Path to download dir
    const std::filesystem::path path_to_dwnld_dir = download_dirs[i];
    for (;;)
    {
        //Rewind
        message.seekp(0);
        message << "Enter target for request (\":q\" to close connection): (" << std::this_thread::get_id() << ")\n";
        target.append(msgng.synch_cin(message.view(), true));
        if (target == "/:q")
        {
            break;
        }
        auto host_field = host_address[i] + ":" + port_service[i];
        //Set up an HTTP GET request message
        b_b_http::request<b_b_http::string_body> req{b_b_http::verb::get, target, version};
        req.set(b_b_http::field::host, host_field);
        req.set(b_b_http::field::user_agent, client_name);
        //Set the timeout
        stream.expires_after(std::chrono::minutes(1));
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
        //Start with an empty_body parser
        b_b_http::response_parser<b_b_http::empty_body> res0;
        stream.expires_after(std::chrono::minutes(1));
        //Read just the header. Otherwise, the empty_body would generate an error if body octets were received
        b_b_http::async_read_header(stream, buffer, res0, yield[ec]);
        if (ec)
        {
            std::string reason = "Error while receiving header of response from resolved IP address with given host_address: "
+ host_address[i] + ", port_service: " + port_service[i] + ", ";
            fail_report(ec, reason.c_str());
            break;
        }
        //Choose a body for whole response
        //Get the parsed header of response message
        auto res0_msg = res0.get();
        //Store contens of content_type field of response header
        auto c_t_field = res0_msg[b_b_http::field::content_type];
        if ((res0_msg[b_b_http::field::content_disposition]).empty() &&
                (c_t_field == "text/html" || c_t_field == "text/plain"))
        {
            b_b_http::response_parser<b_b_http::string_body> res{std::move(res0)};
            b_b_http::async_read(stream, buffer, res, yield[ec]);
            if (ec)
            {
                std::string reason = "Error while receiving whole response from resolved IP address with given host_address: "
    + host_address[i] + ", port_service: " + port_service[i] + ", ";
                fail_report(ec, reason.c_str());
                break;
            }
            //Write the message to standard out
            msgng.synch_cout(res.release(), std::cout);
        } else  //Here we have file to load
        {
            b_b_http::response_parser<b_b_http::file_body> res{std::move(res0)};
            auto set = set_params_file_body_parser(res, path_to_dwnld_dir);
            if (!set)
            {
                break;
            }
            b_b_http::async_read(stream, buffer, res, yield[ec]);
            if (ec)
            {
                std::string reason = "Error while receiving whole response with file from resolved IP address with given host_address: "
    + host_address[i] + ", port_service: " + port_service[i] + ", ";
                fail_report(ec, reason.c_str());
                break;
            }
            //Write the message header to standard out
            msgng.synch_cout(res.get().base(), std::cout);
        }
        target.resize(0);   //Clearing prevoius target
        target.push_back('/');
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

bool Client_HTTP::set_params_file_body_parser(b_b_http::response_parser<b_b_http::file_body>& res,
                                              const std::filesystem::path& path_to_dwnld_dir)
{
    boost::system::error_code ec;
    static thread_local std::filesystem::path path_to_file;
    //Store contens of content_disposition field of response
    auto c_d_field = res.get()[b_b_http::field::content_disposition];
    std::string filename;
    if (c_d_field.empty())
    {
        filename = msgng.synch_cin("Enter name for file to save: ", true);
    } else {
        //c_d_field.remove_suffix(1);
        auto begin = c_d_field.find_first_of('=');
        begin += 2;
        auto end = c_d_field.find_first_of('\"', begin);
        //filename = c_d_field.substr(begin, end).to_string();
        filename = std::string(&c_d_field[begin], &c_d_field[end]);
    }
    path_to_file = path_to_dwnld_dir / filename;
    if (!std::filesystem::exists(path_to_dwnld_dir))
    {
        msgng.synch_cout("Download directory doesn't exist, it will be created now", std::cout);
        std::error_code std_ec;
        std::filesystem::create_directories(path_to_dwnld_dir, std_ec);
        if (std_ec)
        {
            fail_report(std_ec, "Failed to create directory for download: ");
            return false;
        }
    }
    res.get().body().open(path_to_file.c_str(), b_b::file_mode::write, ec);
    //Actually these check is redundant, but just in case
    if(ec == b_b::errc::no_such_file_or_directory)
    {
        fail_report(ec, "Directory  for download doesn't exist");
        return false;
    }
    //Establish limit size for file_body
    if (auto body_lim = res.content_length_remaining())
    {
        res.body_limit(body_lim);
    } else {
        res.body_limit(std::numeric_limits<std::uint64_t>::max());
    }
    return true;
}
}   //namespace client_http
