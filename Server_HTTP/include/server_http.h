#include <string_view>
#include <string>
#include <memory>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/string_body.hpp>


#include <chrono>
#include <iostream>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/beast/core/bind_handler.hpp>

namespace b_b_http = boost::beast::http;

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

class Timer : public boost::asio::coroutine, public std::enable_shared_from_this<Timer>
{
private:
    boost::asio::steady_timer timer;
    char c;
    char a{'*'};
public:
    Timer(boost::asio::steady_timer&& timer_, char c_): timer(std::move(timer_)), c(c_) { }
    void run() { wait(); }

private:
#include <boost/asio/yield.hpp>

    void wait(const boost::system::error_code& ec = { }) {
        boost::ignore_unused(ec);
        //std::shared_ptr<Timer> c = shared_from_this();
        reenter(this)
        {
            std::cout << "After first enter: " << shared_from_this().use_count() << c << std::endl;
            yield timer.async_wait(boost::beast::bind_front_handler(&Timer::wait, std::move(*this)));
            //std::cout << "After yield enter: " << shared_from_this().use_count() << c << std::endl;
        }
        std::cout << a << c << " address of c: " << static_cast<void*>(&c) << std::endl;
    }

#include <boost/asio/unyield.hpp>
};



