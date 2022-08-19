#ifndef SERVER_HTTP_H
#define SERVER_HTTP_H

#include <string_view>
#include <string>
#include <memory>
#include <map>
#include <filesystem>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/string_body.hpp>

#include "parser.h"

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

namespace b_b_http = boost::beast::http;

class Server_HTTP
{
private:
    std::shared_ptr<Mime_types> mime_type;
    std::shared_ptr<Find_file> find_file;

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



