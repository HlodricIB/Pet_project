#ifndef CLIENT_HTTP_H
#define CLIENT_HTTP_H

#define BOOST_ALLOW_DEPRECATED_HEADERS  //To silence warning that <boost/detail/scoped_enum_emulation.hpp> is deprecated

#include <thread>
#include <memory>
#include <vector>
#include <iostream>
#include <string_view>
#include <mutex>
#include <atomic>
#include <syncstream>
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
#include <oneapi/tbb/concurrent_queue.h>
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
    DOWNLOAD_DIR,
    TARGETS
};

namespace b_a = boost::asio;
namespace b_b = boost::beast;
namespace b_b_http = b_b::http;

class Client_HTTP
{
    using size_v_s = std::vector<std::string>::size_type;
private:
    struct Messaging
    {
    private:
        Client_HTTP& outer_self_ptr;
        std::mutex im;
        std::osyncstream sync_cout{std::cout};
        std::osyncstream sync_cerr{std::cerr};
        //std::mutex om;
    public:
        Messaging(Client_HTTP& outer_self_ptr_): outer_self_ptr(outer_self_ptr_) { }
        template<typename Message>
        void
        synch_cout(const Message message, bool if_sync_cout)
        {
            static std::string target_message{"\nEnter target for request (\":q\" to close connection): ("};
            std::thread::id _id;
            outer_self_ptr.ids_queue.try_pop(_id);
            if (if_sync_cout)
            {
                //std::lock_guard<std::mutex> lock(om);
                sync_cout << message << std::flush;
                sync_cout.emit();
            } else {
                sync_cerr << message << std::flush;
                sync_cerr.emit();
            }
            if (outer_self_ptr.ids_queue.try_pop(_id))
            {
                //std::lock_guard<std::mutex> lock(om);
                std::lock_guard<std::mutex> lock(im);
                sync_cout << target_message << _id << ')' << std::endl;
                sync_cout.emit();
            }
        }
        template<typename Message>
        std::string
        synch_cin(const Message message, bool if_cout)
        {
            static thread_local std::string target;
            outer_self_ptr.ids_queue.push(std::this_thread::get_id());
            std::lock_guard<std::mutex> lock(im);
            {
                if (if_cout)
                {
                    //std::scoped_lock<std::mutex, std::mutex> lock(om, im);
                    sync_cout << message << std::flush;
                    sync_cout.emit();
                }
                //std::lock_guard<std::mutex> lock(im);
                std::getline(std::cin, target);
            }
            return target;
        }
    };
    int num_threads{0};
    b_a::io_context ioc;
    b_b::string_view client_name;
    Messaging msgng;
    std::vector<std::string> host_address, port_service, download_dirs, targets;
    int version;
    std::atomic<int> targets_loops{0}; //Tells how many times to loop through the target vector
    std::atomic<size_v_s> targets_index{0}; //For current index in the std::vector<std::string> targets tracking
    std::vector<std::thread> ioc_threads;   //Threads for ioc to run
    oneapi::tbb::concurrent_queue<std::thread::id> ids_queue;
    void parse(const std::string_view&, std::vector<std::string>&);
    template<typename Error_code>
    void
    fail_report(Error_code ec, const char* reason) { msgng.synch_cout(std::string(reason + ec.message()), false); }
    void do_session(size_v_s, b_a::yield_context);
    bool set_params_file_body_parser(b_b_http::response_parser<b_b_http::file_body>&, const std::filesystem::path&); //Second argument is a reference to download dir the appropriate
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
