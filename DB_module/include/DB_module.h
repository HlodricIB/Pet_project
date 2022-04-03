#ifndef DB_MODULE_H
#define DB_MODULE_H

#include <iostream>
#include <deque>
#include <memory>
#include <mutex>
#include <future>
#include <vector>
#include "libpq-fe.h"
#include "parser.h"

class connection_pool
{
private:
    std::deque<PGconn*> conns_deque;    // Deque is because we need pop operations and access to specified element via [ ]
    std::mutex mut;
public:    
    connection_pool() { }
    explicit connection_pool(std::shared_ptr<Parser> parser);
    connection_pool(connection_pool&& other): conns_deque(std::move(other.conns_deque)) { }
    ~connection_pool();
    PGconn* pull_connection() { std::lock_guard<std::mutex> lk(mut); auto conn = conns_deque.front(); conns_deque.pop_front(); return conn; };
    void push_connection(PGconn* conn) { std::lock_guard<std::mutex> lk(mut); conns_deque.push_back(conn); }
    bool is_empty() const { return conns_deque.empty(); }

    size_t c_p_size() const { return conns_deque.size(); }

    //For standalone module only:
    explicit connection_pool(const char* conninfo = "dbname = pet_project_db");

};

using future_results = std::future<std::vector<PGresult*>>;

class DB_module
{
private:
    std::shared_ptr<Parser> parser; // For establishing new connections to database if we need
    std::shared_ptr<connection_pool> conns; //Shared_ptr to pool of established connections
    std::vector<PGresult*> async_command_execution(PGconn*, bool);
public:
    DB_module(std::shared_ptr<Parser> parser_): parser(parser_) { };
    ~DB_module();
    future_results exec_command(const char*);

    //For standalone module only:
    DB_module(const char* conninfo = "dbname = pet_project_db");
};

void display_exec_result(future_results);

#endif // DB_MODULE_H
