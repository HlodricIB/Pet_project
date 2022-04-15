#include <thread>
#include <chrono>
#include "DB_module.h"

//*******************************************DB_module*******************************************

//DB_module::DB_module(std::shared_ptr<Parser> p_DB)
DB_module::DB_module(const char* conninfo)
{
    auto thread_count = std::thread::hardware_concurrency();
    auto conn_threads_count = thread_count > 0 ? (thread_count * 2) : 1;  //Amount of PGconn*
    //conns = std::make_shared<connection_pool>(conn_count, p_DB);
    conns = std::make_shared<connection_pool>(conn_threads_count, conninfo);
    threads = std::make_shared<thread_pool>(conn_threads_count);

};

future_result DB_module::exec_command(const char* command) const
{
    /*PGconn* conn;
    //while (!conns->pull_connection(conn));
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
    std::promise<shared_PG_result> PG_result_promise;
    future_result res = PG_result_promise.get_future();
    auto send_success = PQsendQuery(conn, command);
    if (send_success != 1)  // If sending command to Postgre server was not succesfull
    {
        //std::cerr << "Command failed" <<std::endl;
        PG_result_promise.set_value(nullptr);   //To return an exception object of type std::future_error with an error condition std::future_errc::broken_promise into res
    } else {
        auto lambda = [this, PG_result_promise = std::move(PG_result_promise), conn] () mutable {
            std::vector<PGresult*> vec_res;
            PGresult* single_res{0};
            while ((single_res = PQgetResult(conn)))
            {
                if (PQnfields(single_res) == 0)
                {
                    PQclear(single_res);
                } else
                {
                    vec_res.push_back(single_res);
                }
            }
            conns->push_connection(conn);   // Return connection to pool of connections
            shared_PG_result res = std::make_shared<PG_result>(vec_res);
            PG_result_promise.set_value(res);
        };
        threads->push_task(std::move(lambda));
    }
    return res; // Returning the future, that contains results from the promise*/

    std::promise<shared_PG_result> PG_result_promise;
    future_result res = PG_result_promise.get_future();
        auto lambda = [this, PG_result_promise = std::move(PG_result_promise), command] () mutable {
            PGconn* conn;
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
            shared_PG_result res = std::make_shared<PG_result>(PQexec(conn, command));
            conns->push_connection(conn);   // Return connection to pool of connections
            PG_result_promise.set_value(res);
        };
        threads->push_task(std::move(lambda));
    return res; // Returning the future, that contains results from the promise
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
            std::cerr << "Creating pool of threads failed\n";
        }
    }
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
        data_cond.wait(lk, [this] () -> bool { return (!task_deque.empty()) || done;});   //Don't want to loop here, I'll wait for notifying condition variable
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

//connection_pool::connection_pool(std::shared_ptr<Parser> parser)
connection_pool::connection_pool(size_t conn_count, const char* conninfo)
{
    int attempts_overall = 20;  //Overall attempts to get needed amount of PGconn*
    std::deque<std::pair<std::future<bool>, PGconn*>> temp_conn;   // Temporary deque to store PGconn* that are not real connections at that time
    PGconn* conn;
    for (size_t i = 0; i != conn_count; ++i)
    {
        --attempts_overall;
        //PGconn* conn = PQconnectStartParams(p_DB.parsed_info_ptr('k'), p_DB.parsed_info_ptr('v'), 0);
        conn = PQconnectStart(conninfo);    // First of all getting pointers to PGconn which are not real connections at that time
        if (conn)
        {
            PQsetnonblocking(conn, 1);  //Setting conn connection to nonblocking connection;
            temp_conn.push_back(std::pair(std::future<bool>(), conn));    // PGconn* conn is not a real connection at that time
        } else
        {
            --i;   // One more try
        }
        if (attempts_overall == 0)
        {
            break;  // Attempts to get PGconn* are over
        }
    }
    for (auto& t_conn : temp_conn)
    {
        PGconn* conn = t_conn.second;
        {
            // Tying to establish real connections related to relevant PGconn
            std::future<bool> is_established = std::async([conn] ()->bool {
                namespace c = std::chrono;
                auto const timeout = c::steady_clock::now() + c::minutes(1); // Establishing waiting connection time to avoid endless loop in while below
                enum { PGRES_POLLING_OK = 3};   // To make comparison with returnig value_type of PQconnectPoll
                while (timeout > c::steady_clock::now())
                {
                    if (PQconnectPoll(conn) == 3)
                    {
                        return true;
                    }
                }
                return false;
            });
            t_conn.first = std::move(is_established);
        }
    }
    for (auto& t_conn : temp_conn)
    {
        if ((t_conn.first).get())   // If real connection was established, than related PGconn* from temp_conn have to be copied to conns_deque
        {
            conns_deque.push_back(t_conn.second);
        } else
        {
            PQfinish(t_conn.second);
        }
    }
}

connection_pool::~connection_pool()
{
    while (!conns_deque.empty())
    {
        PQfinish(conns_deque.front());
        conns_deque.pop_front();
    }
}

bool connection_pool::pull_connection(PGconn*& conn)
{
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

//*******************************************PG_result*******************************************

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

PG_result::~PG_result()
{
    PQclear(result);
}


