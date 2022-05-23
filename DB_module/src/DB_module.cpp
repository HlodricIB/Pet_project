#include <thread>
#include <chrono>
#include "DB_module.h"

//*******************************************DB_module*******************************************
size_t DB_module::conns_threads_count() const
{
    auto thread_count = std::thread::hardware_concurrency();
    return thread_count > 0 ? (thread_count * 2) : 1;  //Amount of PGconn* and threads
}

DB_module::DB_module(std::shared_ptr<Parser> p_DB)
{
    auto conn_threads_count = conns_threads_count();
    conns = std::make_shared<connection_pool>(conn_threads_count, p_DB, true);
    threads = std::make_shared<thread_pool>(conn_threads_count);
}

DB_module::DB_module(const char* conninfo)
{
    auto conn_threads_count = conns_threads_count();
    conns = std::make_shared<connection_pool>(conn_threads_count, true, conninfo);
    threads = std::make_shared<thread_pool>(conn_threads_count);
}

//std::mutex iom;

future_result DB_module::exec_command(const char* command) const
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
        shared_PG_result res = std::make_shared<PG_result>(PQexec(conn, command));
        /*auto stop3 = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::mutex> lk(iom);
        std::cout << "From lambda: " << std::chrono::duration<double, std::micro>(stop3 - start3).count() << std::endl;
        std::cout << "Connection: " << (void*)conn << "; Thread_id: " << std::this_thread::get_id() << std::endl;
        iom.unlock();*/

        PG_result_promise.set_value(res);
        conns->push_connection(conn);   // Return connection to pool of connections
        };
    threads->push_task(std::move(lambda));
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
    starting_threads(threads_count);
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

void thread_pool::push_task(function_wrapper&& task)
{
    {
        std::lock_guard<std::mutex> lk (mut);
        task_deque.push_back(std::move(task));
    }                       // Scope here to
    data_cond.notify_one(); // notify the condition variable after unlocking the mutex — this is so that, if the waiting thread wakes
                            // immediately, it doesn’t then have to block again, waiting for you to unlock the mutex.
}

//*******************************************connection_pool*******************************************

connection_pool::connection_pool(size_t conn_count, std::shared_ptr<Parser> p_DB, bool warm)
{
    auto connect = [p_DB] ()->PGconn* { return PQconnectdbParams(p_DB->parsed_info_ptr('k'), p_DB->parsed_info_ptr('v'), 0); };
    make_connections(conn_count, connect, warm);
}

connection_pool::connection_pool(size_t conn_count, bool warm, const char* conninfo)
{
    auto connect = [conninfo] ()->PGconn* { return PQconnectdb(conninfo); };
    make_connections(conn_count, connect, warm);
}

void connection_pool::make_connections(size_t conn_count, std::function<PGconn*()> connect, bool warm)
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
            if (warm)
            {
                //auto start3 = std::chrono::high_resolution_clock::now();
                PGresult* warm_song = PQexec(conn, "SELECT * FROM song_table");
                /*auto stop3 = std::chrono::high_resolution_clock::now();
                std::cout << "From lambda: " << std::chrono::duration<double, std::micro>(stop3 - start3).count() << std::endl;
                std::cout << PQresStatus(PQresultStatus(warm_song)) << std::endl;*/
                PQclear(warm_song);
                //auto start4 = std::chrono::high_resolution_clock::now();
                PGresult* warm_log = PQexec(conn, "SELECT * FROM log_table");
                /*auto stop4 = std::chrono::high_resolution_clock::now();
                std::cout << "From lambda: " << std::chrono::duration<double, std::micro>(stop4 - start4).count() << std::endl;
                std::cout << PQresStatus(PQresultStatus(warm_log)) << std::endl;*/
                PQclear(warm_log);
            }
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

//*******************************************PG_result*******************************************
PG_result::~PG_result()
{
    PQclear(result);
}

void PG_result::display_exec_result()
{
    ExecStatusType res_status = PQresultStatus(result);
    if (res_status == PGRES_TUPLES_OK && PQnfields(result) != 0)
    {
        int nFields = PQnfields(result);
        for (int i = 0; i != nFields; ++i)
            std::cout << PQfname(result, i) << '\t';
        std::cout << std::endl;
        int l = PQntuples(result);
        for (int i = 0; i < l; i++)
        {
            {
                for (int j = 0; j < nFields; j++)
                    std::cout << PQgetvalue(result, i, j) << "\t";
            }
        std::cout << std::endl;
        }
    }
}

PG_result& PG_result::operator=(PG_result&& rhs)
{
    if (this != &rhs)
    {
        result = rhs.result;
        rhs.result = nullptr;
    }
    return *this;
}

PG_result::PG_result(PG_result&& other)
{
    result = other.result;
    other.result = nullptr;
}


