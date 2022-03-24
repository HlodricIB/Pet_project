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

void DB_module::exec_command(const char* command)
{
    auto res = PQexec(conn, command);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Command " << command << " failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        exit_nicely();
    }
    int nFields = PQnfields(res);
    for (int i = 0; i != nFields; ++i)
        std::cout << PQfname(res, i) << '\t';
    std::cout << std::endl;
    PQclear(res);
}
