#include <thread>
#include <chrono>
#include "DB_module.h"

//DB_module::DB_module(std::shared_ptr<Parser> p_DB)
DB_module::DB_module(const char* conninfo)
{
    auto thread_count = std::thread::hardware_concurrency();
    auto conn_threads_count = thread_count > 0 ? thread_count : 1;  //Amount of PGconn*
    //conns = std::make_shared<connection_pool>(conn_count, p_DB);
    conns = std::make_shared<connection_pool>(conn_threads_count, conninfo);
    threads = std::make_shared<thread_pool>(conn_threads_count);

};

DB_module::~DB_module()
{
    this->exec_command("TRUNCATE song_table RESTART IDENTITY");
    this->exec_command("DELETE FROM log_table; ALTER SEQUENCE log_table_id_seq RESTART WITH 1");
}

future_result DB_module::exec_command(const char* command) const
{
    PGconn* conn;
    while (conns->pull_connection(conn));
    std::promise<shared_PG_result> PG_result_promise;
    future_result res = PG_result_promise.get_future();
    auto send_success = PQsendQuery(conn, command);
    if (send_success != 1)  // If sending command to Postgre server was not succesfull
    {
        //std::cerr << "Command failed" <<std::endl;
        PG_result_promise.~promise();   //To return an exception object of type std::future_error with an error condition std::future_errc::broken_promise into res
        return res;
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
            conns->push_connection(conn);
            shared_PG_result res = std::make_shared<PG_result>(vec_res);
            PG_result_promise.set_value(res);
            conns->push_connection(conn);   // Return connection to pool of connections
        };
        threads->push_task(std::move(lambda));
    }
    return res;
}

thread_pool::thread_pool()
{
    auto thread_count = std::thread::hardware_concurrency();
    auto threads_count = thread_count > 0 ? thread_count : 1;  //Amount of threads
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

//connection_pool::connection_pool(std::shared_ptr<Parser> parser)
connection_pool::connection_pool(size_t conn_count, const char* conninfo)
{
    int attempts_overall = 20;  //Overall attempts to get needed amount of PGconn*
    std::deque<std::pair<std::future<bool>, PGconn*>> temp_conn;   // Temporary deque to store PGconn* that are not real connections at that time
    for (size_t i = 0; i != conn_count; ++i)
    {
        --attempts_overall;
        //PGconn* conn = PQconnectStartParams(p_DB.parsed_info_ptr('k'), p_DB.parsed_info_ptr('v'), 0);
        PGconn* conn = PQconnectStart(conninfo);    // First of all getting pointers to PGconn which are not real connections at that time
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

void PG_result::display_exec_result()
{
    for (auto& res : results)
    {
        ExecStatusType res_status = PQresultStatus(res);
        if (res_status != PGRES_TUPLES_OK && res_status != PGRES_COMMAND_OK)
        {
            std::cerr << "Command failed: " << PQresultErrorMessage(res) << std::endl;
        }
        if (res_status == PGRES_TUPLES_OK)
        {
            int nFields = PQnfields(res);
            for (int i = 0; i != nFields; ++i)
                std::cout << PQfname(res, i) << '\t';
            std::cout << std::endl;
            int l = PQntuples(res);
            for (int i = 0; i < l; i++)
            {
                {
                    for (int j = 0; j < nFields; j++)
                        std::cout << PQgetvalue(res, i, j) << "\t";
                }
            std::cout << std::endl;
            }
        }
    }
}


