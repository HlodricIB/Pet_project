#include <iostream>
#include "server_http.h"

#include <cassert>

int main()
{
    /*std::string filename;
    filename = full_filename("song2*");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;
    filename = full_filename("song2.wav");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;
    filename = full_filename("song1*");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;
    filename = full_filename("s");
    std::cout << filename << '\t' << mime_type(filename) << std::endl;*/

    boost::asio::io_context ioc{};
    boost::asio::steady_timer timer1(ioc, std::chrono::steady_clock::now() + std::chrono::seconds(10));
    boost::asio::steady_timer timer2(ioc, std::chrono::steady_clock::now() + std::chrono::seconds(10));
    std::make_shared<Timer>(std::move(timer1), 'r')->run();
    auto p = std::make_shared<Timer>(std::move(timer2), 'l');
    p->run();
    assert(p.get() != nullptr);
    std::cout << "Before ioc.run: " << p.use_count() << std::endl;
    ioc.run();
    assert(p.get() == nullptr);
    std::cout << "Before return from main: " << p.use_count() << std::endl;
    return 0;
}
