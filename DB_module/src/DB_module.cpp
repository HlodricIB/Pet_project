#include <thread>
#include <chrono>
#include "DB_module.h"

//DB_module::DB_module(const Parser_DB& p_DB)
DB_module::DB_module(const char* conninfo)
{
    //conns = std::make_shared<connection_pool>(parser);
    conns = std::make_shared<connection_pool>(conninfo);
};

DB_module::~DB_module()
{
    auto res1 = this->exec_command("TRUNCATE song_table RESTART IDENTITY");
    auto res2 = this->exec_command("DELETE FROM log_table; ALTER SEQUENCE log_table_id_seq RESTART WITH 1");
    display_exec_result(std::move(res1));
    display_exec_result(std::move(res2));
}

std::vector<PGresult*> DB_module::async_command_execution(PGconn* conn, bool is_from_pool)
{
    std::vector<PGresult*> res;
    PGresult* single_res{0};
    while ((single_res = PQgetResult(conn)))
    {
        res.push_back(single_res);
    }
    is_from_pool ? conns->push_connection(conn) : PQfinish(conn);
    return res;
}

future_results DB_module::exec_command(const char* command)
{
    PGconn* conn;
    bool is_from_pool{true};   // To know if we established new connection to Postgre server, or took it from connection_pool
    if (!conns->is_empty())
    {
        conn = conns->pull_connection();

    } else
    {
        //conn = PQconnectStartParams(parser->parsed_info_ptr('k'), parser->parsed_info_ptr('v'), 0);
        conn = PQconnectdb("dbname = pet_project_db");
        is_from_pool = false;
    }
    std::cout << conns->c_p_size() << std::endl;
    auto send_success = PQsendQuery(conn, command);
    if (send_success != 1)  // If sending command to Postgre server was not succesfull
    {
        return future_results();
    } else
    {
        future_results res = std::async([this, conn, is_from_pool] ()-> std::vector<PGresult*> {
            return async_command_execution(conn, is_from_pool);});  //Lambda here needed to pass this to return conn into connection_pool or finish conn when results retreived
        return res;
    }
}

void display_exec_result(future_results res)
{
    if (!res.valid())
    {
        return;
    }
    std::vector<PGresult*> res_vector = res.get();
    for (auto iter = res_vector.begin(); iter != res_vector.end(); ++iter)
    {
        ExecStatusType res_status = PQresultStatus(*iter);
        if (res_status != PGRES_TUPLES_OK && res_status != PGRES_COMMAND_OK)
        {
            std::cerr << "Command failed: " << PQresultErrorMessage(*iter) << std::endl;
        }
        if (res_status == PGRES_TUPLES_OK)
        {
            int nFields = PQnfields(*iter);
            for (int i = 0; i != nFields; ++i)
                std::cout << PQfname(*iter, i) << '\t';
            std::cout << std::endl;
            int l = PQntuples(*iter);
            for (int i = 0; i < l; i++)
            {
                {
                    for (int j = 0; j < nFields; j++)
                        std::cout << PQgetvalue(*iter, i, j) << "\t";
                }
            std::cout << std::endl;
            }
        }
        PQclear(*iter);
    }
}

//connection_pool::connection_pool(std::shared_ptr<Parser> parser)
connection_pool::connection_pool(const char* conninfo)
{
    auto thread_count = std::thread::hardware_concurrency();
    auto conn_count = thread_count > 0 ? thread_count : 1;  //Amount of PGconn*
    int attempts_overall = 20;  //Overall attempts to get needed amount of PGconn*
    std::vector<PGconn*> temp_conn(conn_count);   // Temporary vector to store PGconn* that are not real connections at that time
    for (size_t i = 0; i != conn_count; ++i)
    {
        --attempts_overall;
        //PGconn* conn = PQconnectStartParams(p_DB.parsed_info_ptr('k'), p_DB.parsed_info_ptr('v'), 0);
        PGconn* conn = PQconnectStart(conninfo);    // First of all getting pointers to PGconn which are not real connections at that time
        if (conn)
        {
            PQsetnonblocking(conn, 1);  //Setting conn connection to nonblocking connection;
            temp_conn[i] = conn;
        } else
        {
            --i;   // One more try
        }
        if (attempts_overall == 0)
        {
            break;  // Attempts to get PGconn* are over
        }
    }
    std::vector<std::future<bool>> temp_is_established(conn_count);    // Temporary vector to store boolean results of establishing real connections pointers to stored in temp_conn
    for (size_t i = 0; i != temp_conn.size(); ++i)
    {
        PGconn* conn = temp_conn[i];
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
            temp_is_established[i] = std::move(is_established);
        }
    }
    for (size_t i = 0; i != conn_count; ++i)
    {
        if (temp_is_established[i].get())   // If real connection was established, than related PGconn* from temp_conn have to be copied to conns_deque
        {
            conns_deque.push_back(temp_conn[i]);
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


