#include <thread>
#include <future>
#include "DB_module.h"

DB_module::DB_module(const Parser_DB& p_DB)
{
    conn = PQconnectdbParams(p_DB.parsed_info_ptr('k'), p_DB.parsed_info_ptr('v'), 0);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        std::cerr << "Unable to connect to Postgres database: " << PQerrorMessage(conn) << std::endl;
        exit_nicely();
    }
};

DB_module::~DB_module()
{
    this->exec_command("TRUNCATE song_table RESTART IDENTITY");
    this->exec_command("DELETE FROM log_table; ALTER SEQUENCE log_table_id_seq RESTART WITH 1");
    PQfinish(conn);
}

void DB_module::exec_command(const char* command)
{
    auto res = PQexec(conn, command);
    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Command " << command << " failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        exit_nicely();
    }
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
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
    PQclear(res);
}

void connection_pool::connection_establishing()
{

}

connection_pool::connection_pool(const char* conninfo)
{
    auto thread_count = std::thread::hardware_concurrency();
    auto conn_count = thread_count > 0 ? thread_count : 1;
    for (unsigned int i = 0; i != conn_count; ++i)
    {
        PGconn* conn =
    }
}


