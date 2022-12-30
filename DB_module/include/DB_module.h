#ifndef DB_MODULE_H
#define DB_MODULE_H

#include <iostream>
#include <deque>
#include <memory>
#include <mutex>
#include <future>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <utility>
#include "libpq-fe.h"
#include "parser.h"

namespace db_module
{
class connection_pool
{
private:
    mutable std::mutex mut;
    std::deque<PGconn*> conns_deque;    //Deque is because we need pop operations
    int conns_established{0};
    void make_connections(size_t conn_count, std::function<PGconn*()>);
public:
    explicit connection_pool(size_t = 1, const char* conninfo = "dbname = pet_project_db");    //Hidden default ctor, first argument is for setting number of connections to open
    connection_pool(size_t, std::shared_ptr<::parser::Parser> parser);   //First argument is for setting number of connections to open
    connection_pool(const connection_pool&) = delete;
    connection_pool& operator=(const connection_pool&) = delete;
    connection_pool(connection_pool&) = delete;
    connection_pool& operator=(connection_pool&) = delete;
    ~connection_pool();
    bool pull_connection(PGconn*&);
    void push_connection(PGconn* conn) { std::lock_guard<std::mutex> lk(mut); conns_deque.push_front(conn); }   //Push_front because sequential queries on same connection is faster than on different
    int conns_amount() const { return conns_established; }
    void warm(const std::string&, const std::vector<std::string>&);    //Function for "warming up" (filling cache for DB) established connections, first argument is DB name to "warming up", second is table for "warming up"
};

class function_wrapper
{
private:
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() { }
    };
    std::unique_ptr<impl_base> impl;
    template<typename F>
    struct impl_type: impl_base {
        F f;
        impl_type(F&& f_): f(std::move(f_)) { }
        void call() override { f(); }
    };
public:
    template<typename F>
    function_wrapper(F&& f): impl(new impl_type<F>(std::move(f))) { }
    void operator()();
    function_wrapper() = default;
    function_wrapper(function_wrapper&& other): impl(std::move(other.impl)) { }
    function_wrapper& operator=(function_wrapper&& other) { impl = std::move(other.impl); return *this; }
    function_wrapper(const function_wrapper&) = delete;
    function_wrapper(function_wrapper&) = delete;
    function_wrapper& operator=(const function_wrapper&) = delete;
};

class thread_pool
{
private:
    mutable std::mutex mut;
    std::condition_variable data_cond;
    std::atomic_bool done{false};
    std::vector<std::thread> threads;
    std::deque<function_wrapper> task_deque;
    int threads_started{0};
    void pull_task(function_wrapper&);  //Reference here is because task already created in void worker_thread() function, that defined below
    void starting_threads(size_t);
    void worker_thread();
public:
    thread_pool();  //Creates thread_pool with threads amount depending on std::thread::hardware_concurrency()
    explicit thread_pool(size_t);    // If you want to create thread_pool with specified threads amount, throws std::logic_error if specified amount is less than or equal to zero
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool(thread_pool&) = delete;
    thread_pool& operator=(thread_pool&) = delete;
    ~thread_pool();
    void push_task_back(function_wrapper&&);
    void push_task_front(function_wrapper&&);
    int threads_amount() const { return threads_started; }
};

class PG_result
{
private:
    static std::vector<std::string> not_succeed;
    PGresult* result{nullptr};
    std::string DB_name;    //Database name from which the result was obtained
    bool success{false};
    int nFields{0};
    int nTuples{0};
    void construct_assign(PG_result&&);
public:
    using result_container = std::vector<std::vector<std::pair<const char*, size_t>>>;
    PG_result() { }
    PG_result(PGresult*, const std::string);    //Second argument is database name that PGresult* is related to
    PG_result(PG_result&&);
    PG_result& operator=(PG_result&&);
    ~PG_result();
    void display_exec_result();
    const PGresult* get_result_ptr() const { return result; }
    result_container get_result_container() const;
    int get_result_command_tag() const;
    char* get_result_single() const { return PQgetvalue(result, 0, 0); }
    int get_columns_number() const { return nFields; }
    int get_rows_number() const { return nTuples; }
    const std::string res_error() const;
    bool res_succeed() const;
    const std::string& res_DB_name() const { return DB_name; }
};

using shared_PG_result = std::shared_ptr<PG_result>;
using future_result = std::future<shared_PG_result>;

class DB_module
{
private:
    std::condition_variable conn_cond;
    std::shared_ptr<connection_pool> conns{nullptr}; //Shared_ptr to pool of established connections
    std::shared_ptr<thread_pool> threads{nullptr};   //Shared_ptr to pool of threads
    shared_PG_result async_command_execution(const char*) const;
    size_t conns_threads_count() const;
public:
    explicit DB_module(std::shared_ptr<::parser::Parser>);
    explicit DB_module(const char* conninfo = "dbname = pet_project_db");
    DB_module(std::shared_ptr<connection_pool> c_pool, std::shared_ptr<thread_pool> t_pool): conns(c_pool), threads(t_pool) { }   // If want to use our own created connection and thread pools with specified connections and threads amount
    ~DB_module() { };
    future_result exec_command(const std::string&, bool) const;    //First argument is the command to execute, second shows if command have to be executed out of turn
    std::pair<int, int> conns_threads_amount() const;
    void warming_connections(const std::string& DB_name, const std::vector<std::string>& tables) { conns->warm(DB_name, tables); } //Function for "warming up" (filling cache for DB) established connections in connection pool, first argument is DB name to "warming up", second is table for "warming up"
};
}   //namespace db_module

#endif // DB_MODULE_H
