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

class thread_pool
{
private:
    std::deque<std::function<void()>> task_deque;
    std::mutex mut;
    std::condition_variable data_cond;
    std::atomic_bool done{false};
    void pull_task(std::function<void()>& task) {
        std::lock_guard<std::mutex> lk (mut);
            task = task_deque.front();
            task_deque.pop_front();
        }
public:
    thread_pool();
    thread_pool(size_t);    // If you want to create thread_pool with specified connections amount
    void worker_thread()
    {
        std::function<void()> task;
        while(!done)
        {
            std::unique_lock<std::mutex> lk(mut);
            data_cond.wait(lk, [this] { return this->empty();} );   //Don't want to loop here, I'll wait for notifying condition variable
            pull_task(task);
            task();
        }
    }
    bool empty() { std::lock_guard<std::mutex> lk (mut); return task_deque.empty(); }
    void push_task(std::function<void()>&& task) { std::lock_guard<std::mutex> lk (mut); task_deque.push_back(task); data_cond.notify_one(); }
    std::condition_variable& cond() { return data_cond; }
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
