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

class connection_pool
{
private:
    std::deque<PGconn*> conns_deque;    // Deque is because we need pop operations
    mutable std::mutex mut;
public:    
    connection_pool() { }
    explicit connection_pool(size_t, std::shared_ptr<Parser> parser);   //First argument is for setting number of connections to open
    connection_pool(connection_pool&& other): conns_deque(std::move(other.conns_deque)) { }
    ~connection_pool();
    bool pull_connection(PGconn*& conn) {
        std::lock_guard<std::mutex> lk(mut);
        if (conns_deque.empty())
        {
            return false;
        } else
        {
            conn = conns_deque.front();
            conns_deque.pop_front();
            return true;
        }
    }
    void push_connection(PGconn* conn) { std::lock_guard<std::mutex> lk(mut); conns_deque.push_back(conn); }

    //For standalone module only:
    explicit connection_pool(size_t, const char* conninfo = "dbname = pet_project_db");    //First argument is for setting number of connections to open

};

class function_wrapper
{
private:
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() { }
    };
    std::unique_ptr<impl_base> impl;
    template< typename F>
    struct impl_type: impl_base {
        F f;
        impl_type(F&& f_): f(std::move(f_)) { }
        void call() override { f(); }
    };
public:
    template<typename F>
    function_wrapper(F&& f): impl(new impl_type<F>(std::move(f))) { }
    void operator()() { impl->call(); }
    function_wrapper() = default;
    function_wrapper(function_wrapper&& other): impl(std::move(other.impl)) {  }
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
    void pull_task(function_wrapper& task) {   //Reference here is because task already created in void worker_thread() function, that defined below
        task = std::move(task_deque.front());
        task_deque.pop_front();
        }
    void starting_threads(size_t);
public:
    thread_pool();
    thread_pool(size_t);    // If you want to create thread_pool with specified connections amount
    ~thread_pool();
    void worker_thread()
    {
        function_wrapper task;
        while(!done)
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk, [this] () -> bool { return (done || !task_deque.empty());});   //Don't want to loop here, I'll wait for notifying condition variable
            if (done)
                break;
            pull_task(task);
            task();
        }
    }
    bool empty() { std::lock_guard<std::mutex> lk (mut); return task_deque.empty(); }
    void push_task(function_wrapper task) { std::lock_guard<std::mutex> lk (mut); task_deque.push_back(std::move(task)); data_cond.notify_one(); }
};

class PG_result
{
private:
    std::vector<PGresult*> results; //Container is because one query may return several results
public:
    PG_result() { }
    PG_result(const std::vector<PGresult*>& res): results(res) { }
    ~PG_result()
    {
        for(auto& res : results)
        {
            PQclear(res);
        }
    }
    void display_exec_result();
    PGresult* get_result(int i) { return results[i]; }
};

using shared_PG_result = std::shared_ptr<PG_result>;
using future_result = std::future<shared_PG_result>;

class DB_module
{
private:
    std::shared_ptr<connection_pool> conns; //Shared_ptr to pool of established connections
    std::shared_ptr<thread_pool> threads;   //Shared_ptr to pool of threads
    shared_PG_result async_command_execution(const char*) const;
public:
    DB_module(std::shared_ptr<Parser> parser_);
    DB_module(std::shared_ptr<connection_pool> c_pool): conns(c_pool) { }   // If want to use our own created pool with specified connections amount
    ~DB_module();
    future_result exec_command(const char*) const;

    //For standalone module only:
    DB_module(const char* conninfo = "dbname = pet_project_db");
};


#endif // DB_MODULE_H
