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
//#include <oneapi/tbb/concurrent_queue.h>
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
        std::mutex im;
        std::osyncstream sync_cout{std::cout};
        std::osyncstream sync_cerr{std::cerr};
        std::atomic<std::thread::id> current_cin_thread;    //Id of the thread that is waiting at the current moment for
                                                            //target to be entered
    public:
        Messaging() { }
        template<typename Message>
        void
        synch_cout(const Message message, bool if_sync_cout, const char* extra_char = "")
        {
            static const std::string target_message{"Enter target for request (\":q\" to close connection): ("};
            static const auto empty_id{std::thread::id()};
            if (if_sync_cout)
            {
                sync_cout << message << extra_char << std::flush_emit;
                //sync_cout.emit();
            } else {
                sync_cerr << message << extra_char << std::flush_emit;
                //sync_cerr.emit();
            }
            if (current_cin_thread.load() != empty_id)
            {
                sync_cout << target_message << current_cin_thread.load() << ")\n" << std::flush_emit;
                //sync_cout.emit();

            }
        }
        template<typename Message>
        std::string
        synch_cin(const Message message, bool if_cout, const char* extra_char = "")
        {
            static thread_local std::string target;
            static const auto empty_id{std::thread::id()};
            std::lock_guard<std::mutex> lock(im);
            {
                if (if_cout)
                {
                    sync_cout << message << extra_char << std::flush_emit;
                    //sync_cout.emit();
                }
                current_cin_thread.store(std::this_thread::get_id());
                std::getline(std::cin, target);
                current_cin_thread.store(empty_id);
            }
            return target;
        }
    };
    int num_threads{0};
    std::atomic_int targets_loops{0}; //Tells how many times to loop through the target vector
    std::atomic<size_v_s> targets_index{0}; //For current index in the std::vector<std::string> targets tracking
    std::atomic_bool targets_looping_over{true};
    int version{0};
    b_a::io_context ioc;
    b_b::string_view client_name;
    Messaging msgng;
    std::vector<std::string> host_address, port_service, download_dirs, targets;
    std::vector<std::thread> ioc_threads;   //Threads for ioc to run
    void parse(const std::string_view&, std::vector<std::string>&);
    template<typename Error_code>
    void
    fail_report(Error_code ec, const char* reason) { msgng.synch_cout(std::string(reason + ec.message() +'\n'), false); }
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
