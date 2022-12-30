#include <thread>
#include <chrono>
#include <cstring>
#include <climits>
#include <stdexcept>
#include "DB_module.h"

namespace db_module
{
//*******************************************DB_module*******************************************
size_t DB_module::conns_threads_count() const
{
    auto thread_count = std::thread::hardware_concurrency();
    //return thread_count > 0 ? (thread_count * 2) : 1;  //Amount of PGconn* and threads
    return thread_count > 0 ? thread_count : 1;  //Amount of PGconn* and threads
}

using namespace parser;

DB_module::DB_module(std::shared_ptr<Parser> p_DB)
{
    auto conn_threads_count = conns_threads_count();
    conns = std::make_shared<connection_pool>(conn_threads_count, p_DB);
    threads = std::make_shared<thread_pool>(conn_threads_count);
}

DB_module::DB_module(const char* conninfo)
{
    auto conn_threads_count = conns_threads_count();
    conns = std::make_shared<connection_pool>(conn_threads_count, conninfo);
    threads = std::make_shared<thread_pool>(conn_threads_count);
}

//std::mutex iom;

future_result DB_module::exec_command(const std::string& command, bool front_flag) const
{
    std::promise<shared_PG_result> PG_result_promise;
    future_result res = PG_result_promise.get_future();
    auto lambda = [this, PG_result_promise = std::move(PG_result_promise), command] () mutable {
        PGconn* conn{nullptr};
        while (true)
        {
            if (conns->pull_connection(conn))
            {
                break;
            } else
            {
                std::this_thread::yield();
            }
        }
        //auto start3 = std::chrono::high_resolution_clock::now();
        shared_PG_result res = std::make_shared<PG_result>(PQexec(conn, command.c_str()), PQdb(conn));
        /*auto stop3 = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::mutex> lk(iom);
        std::cout << "From lambda: " << std::chrono::duration<double, std::micro>(stop3 - start3).count() << std::endl;
        std::cout << "Connection: " << (void*)conn << "; Thread_id: " << std::this_thread::get_id() << std::endl;
        iom.unlock();*/
        PG_result_promise.set_value(res);
        conns->push_connection(conn);   // Return connection to pool of connections
        };
    if (front_flag)
    {
        threads->push_task_front(std::move(lambda));
    } else
    {
        threads->push_task_back(std::move(lambda));
    }
    return res; // Returning the future, that contains results from the promise
}

std::pair<int, int> DB_module::conns_threads_amount() const
{
    return std::make_pair<int, int>(conns->conns_amount(), threads->threads_amount());
}

//*******************************************thread_pool*******************************************

thread_pool::thread_pool()
{
    auto thread_count = std::thread::hardware_concurrency();
    auto threads_count = thread_count > 0 ? (thread_count * 2) : 1;  //Amount of threads
    starting_threads(threads_count);
}

thread_pool::thread_pool(size_t threads_count)
{
    if (threads_count > 0)
    {
        starting_threads(threads_count);
    } else {
        throw std::logic_error("Specified threads amount is less than or equal to zero");
    }
}

void thread_pool::starting_threads(size_t threads_count)
{
    for (size_t i = 0; i != threads_count; ++i)
    {
        try {
            threads.push_back(std::thread(&thread_pool::worker_thread, this));
        }  catch (...) {
            continue;
        }
    }
    threads_started = threads.size();
}

thread_pool::~thread_pool()
{
    done = true;
    data_cond.notify_all();
    for (auto& i : threads)
    {
        if (i.joinable())
        {
            i.join();
        }
    }
}

void thread_pool::pull_task(function_wrapper& task)
{
    task = std::move(task_deque.front());
    task_deque.pop_front();
}

void thread_pool::worker_thread()
{
    function_wrapper task;
    while(!done)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] () -> bool { return (!task_deque.empty()) || done; });   //Don't want to loop here, I'll wait for notifying condition variable
        if (done)
            break;
        pull_task(task);
        lk.unlock();
        task();
    }
}

void thread_pool::push_task_back(function_wrapper&& task)
{
    {
        std::lock_guard<std::mutex> lk (mut);
        task_deque.push_back(std::move(task));
    }                       // Scope here to
    data_cond.notify_one(); // notify the condition variable after unlocking the mutex — this is so that, if the waiting thread wakes
                            // immediately, it doesn’t then have to block again, waiting for you to unlock the mutex.
}

void thread_pool::push_task_front(function_wrapper&& task)
{
    {
        std::lock_guard<std::mutex> lk (mut);
        task_deque.push_front(std::move(task));
    }                       // Scope here to
    data_cond.notify_one(); // notify the condition variable after unlocking the mutex — this is so that, if the waiting thread wakes
                            // immediately, it doesn’t then have to block again, waiting for you to unlock the mutex.
}

//*******************************************connection_pool*******************************************

connection_pool::connection_pool(size_t conn_count, std::shared_ptr<Parser> p_DB)
{
    auto keys_ptr = p_DB->parsed_info_ptr('k');
    auto values_ptr = p_DB->parsed_info_ptr('v');
    if (keys_ptr == nullptr || values_ptr == nullptr)
    {
        return ;
    }
    auto connect = [keys_ptr, values_ptr] ()->PGconn* { return PQconnectdbParams(keys_ptr, values_ptr, 0); };
    make_connections(conn_count, connect);
}

connection_pool::connection_pool(size_t conn_count, const char* conninfo)
{
    if (conninfo == nullptr)
    {
        return ;
    }
    auto connect = [conninfo] ()->PGconn* { return PQconnectdb(conninfo); };
    make_connections(conn_count, connect);
}

void connection_pool::make_connections(size_t conn_count, std::function<PGconn*()> connect)
{
    std::vector<std::future<PGconn*>> futures_conns(conn_count);    //Temporary container to storage futures with PGconn*
    for (size_t i = 0; i != conn_count; ++i)
    {
        std::future<PGconn*> conn = std::async([connect] ()->PGconn* {
            int attempts_overall = 5;  //Overall attempts to set connection;
            for (int i = 0; i != attempts_overall; ++i)
            {
                PGconn* conn = connect();
                if (PQstatus(conn) == CONNECTION_OK)
                {
                    PQsetnonblocking(conn, 1);  //Setting conn connection to nonblocking connection;
                    return conn;
                }
                PQfinish(conn);
            }
            return nullptr;
        });
        futures_conns[i] = std::move(conn);
    }
    for (auto& t_conn : futures_conns)
    {
        PGconn* conn = t_conn.get();
        if (conn)   // If connection was established (not nullptr), than it have to be copied to conns_deque
        {
            conns_deque.push_back(conn);
        }
    }
    conns_established = conns_deque.size();
}

connection_pool::~connection_pool()
{
    PGconn* conn{nullptr};
    while (!conns_deque.empty())
    {
        conn = conns_deque.front();
        PQfinish(conn);
        conns_deque.pop_front();
    }
}

bool connection_pool::pull_connection(PGconn*& conn)
{
    std::unique_lock<std::mutex> lk(mut);
    if (conns_deque.empty())
    {
        return false;
    } else
    {
        conn = conns_deque.front();
        conns_deque.pop_front();
        lk.unlock();
        if (PQstatus(conn) == CONNECTION_OK)    //Checking if connection is still ok
        {
           return true;
        } else
        {
            PQreset(conn);
            if (PQstatus(conn) == CONNECTION_OK)    //Checking if a new connection was established instead of lost one
            {
               return true;
            } else
            {
                return false;
            }
        }
    }
}

void connection_pool::warm(const std::string& DB_name, const std::vector<std::string>& tables)
{
    std::string warming_command;
    for (auto& conn : conns_deque)
    {
        if (std::string(PQdb(conn)) == DB_name)
        {
            for (const auto& table : tables)
            {
                warming_command = "SELECT * FROM " + table;
                PGresult* warm_table = PQexec(conn, warming_command.c_str());
                PQclear(warm_table);
            }
        }
    }
}

//*******************************************function_wrapper*******************************************
void function_wrapper::operator()()
{
    if (impl)
    {
        impl->call();
    }
    return ;
}

//*******************************************PG_result*******************************************
std::vector<std::string> PG_result::not_succeed{"PGRES_EMPTY_QUERY", "PGRES_BAD_RESPONSE", "PGRES_NONFATAL_ERROR", "PGRES_FATAL_ERROR"};

PG_result::PG_result(PGresult* res, const std::string DB_name_): result(res), DB_name(DB_name_)
{
    ExecStatusType status = PQresultStatus(result);
    std::string string_status{PQresStatus(status)};
    auto succeed = std::find(not_succeed.begin(), not_succeed.end(), string_status);
    if (succeed == not_succeed.end())
    {
        success = true;
        nFields = PQnfields(result);
        nTuples = PQntuples(result);
    }
}

PG_result::~PG_result()
{
    PQclear(result);
}

void PG_result::construct_assign(PG_result&& other)
{
    result = other.result;
    other.result = nullptr;
    success = std::move(other.success);
    other.success = false;
    DB_name = std::move(other.DB_name);
    nFields = std::move(other.nFields);
    nTuples = std::move(other.nTuples);
}

PG_result& PG_result::operator=(PG_result&& rhs)
{
    if (this != &rhs)
    {
        construct_assign(std::move(rhs));
    }
    return *this;
}

PG_result::PG_result(PG_result&& other)
{
    construct_assign(std::move(other));
}

void PG_result::display_exec_result()
{
    if (success)
    {
        for (int i = 0; i != nFields; ++i)
            std::cout << PQfname(result, i) << '\t';
        std::cout << std::endl;
        for (int i = 0; i < nTuples; i++)
        {
            {
                for (int j = 0; j < nFields; j++)
                    std::cout << PQgetvalue(result, i, j) << "\t";
            }
        std::cout << std::endl;
        }
    }
}

PG_result::result_container PG_result::get_result_container() const
{
    using inner_result_container = std::vector<std::pair<const char*, size_t>>;
    result_container outer_res_cntnr;
    outer_res_cntnr.reserve(nTuples);
    inner_result_container inner_res_cntnr;
    inner_res_cntnr.reserve(nFields);
    //Forming table header
    const char* ptr{nullptr};
    std::size_t len{0};
    for (inner_result_container::size_type i = 0; i != nFields; ++i)
    {
        ptr = PQfname(result, i);
        len = std::strlen(ptr);
        inner_res_cntnr.emplace_back(std::make_pair<const char*, size_t>(std::move(ptr), std::move(len)));
    }
    outer_res_cntnr.emplace_back(std::move(inner_res_cntnr));
    //Forming table content
    for (result_container::size_type i = 0; i != nTuples; ++i)
    {
        inner_res_cntnr.reserve(nFields);
        for (inner_result_container::size_type j = 0; j != nFields; ++j)
        {
            ptr = PQgetvalue(result, i, j);
            len = std::strlen(ptr);
            inner_res_cntnr.emplace_back(std::make_pair<const char*, size_t>(std::move(ptr), std::move(len)));
        }
        outer_res_cntnr.emplace_back(std::move(inner_res_cntnr));
    }
    return outer_res_cntnr;
}

int PG_result::get_result_command_tag() const
{
    if (result)
    {
        return std::atoi(PQcmdStatus(result));
    } else {
        return std::numeric_limits<int>::max();
    }
}

const std::string PG_result::res_error() const
{
    return std::string(PQresultErrorMessage(result));
}

bool PG_result::res_succeed() const
{
    return success;
}
}   //namespace db_module

