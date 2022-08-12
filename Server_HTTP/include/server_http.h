#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include <string_view>
#include <string>
#include <memory>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/string_body.hpp>


//#include <chrono>
//#include <iostream>
//#include <boost/asio/steady_timer.hpp>
//#include <boost/asio/coroutine.hpp>
//#include <boost/beast/core/bind_handler.hpp>

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
    std::string_view mime_type(std::string_view) override;
};

class Find_file
{
private:

};

namespace b_b_http = boost::beast::http;

std::string_view mime_type(std::string_view);
std::string full_filename(std::string_view);

class Server_HTTP
{
private:
    std::string_view mime_type(std::string_view);
    std::string full_filename(std::string_view);

    template<class Body, class Allocator, class Sender>
    void
    handle_request(std::string_view filename_base, b_b_http::request<Body, b_b_http::basic_fields<Allocator>>&& req, Sender&& sender)
    {
        boost::ignore_unused(filename_base);
        boost::ignore_unused(req);
        boost::ignore_unused(sender);
    }
public:
    Server_HTTP ();
};

#endif // SERVER_HTTP_H



