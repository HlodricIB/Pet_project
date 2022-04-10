#include <chrono>   // To check perfomance gain from current realization of DB_module
#include "libpq-fe.h"
#include "DB_module.h"

#include <thread>

void display_exec_result_sync(PGresult*);
//void display_exec_result_async(PGresult*);
void fill_table(const char*);
double async_handle(const DB_module&, std::vector<std::string>&, future_result*);
double one_threaded_handle(std::vector<std::string>&, PGresult**);

int main()
{
    /*auto res1 = db.exec_command("BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song1', 001); COMMIT");
    auto res2 = db.exec_command("BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song2', 002); COMMIT");
    auto res3 = db.exec_command("BEGIN; SELECT FOR SHARE; SELECT * FROM song_table; COMMIT");
    auto res4 = db.exec_command("BEGIN; SELECT FOR SHARE; SELECT * FROM log_table; COMMIT");*/
    std::vector<std::string> async_query;
    std::string s = "BEGIN; SELECT FOR SHARE; SELECT * FROM song_table; COMMIT";
    for (int i = 0; i != 300; ++i)
    {
        async_query.push_back(s);
    }
    std::vector<std::string> one_threaded_query;
    s = "SELECT * FROM song_table";
    for (int i = 0; i != 300; ++i)
    {
        one_threaded_query.push_back(s);
    }
    double async, one_threaded, getting;
    fill_table("dbname = pet_project_db");
    {
        //std::vector<future_result> async_res;
        future_result async_res_m[async_query.size() + 2];
        DB_module db{ };
        async = async_handle(db, async_query, async_res_m);
        auto start = std::chrono::high_resolution_clock::now();
        shared_PG_result res;
        for (auto& i : async_res_m)
        {
            res = i.get();
            if (res)
                res->display_exec_result();
                //i.get();
        }
        auto stop = std::chrono::high_resolution_clock::now();
        getting = std::chrono::duration<double, std::micro>(stop - start).count();
    }
    {
        fill_table("dbname = pet_project_db");
        //std::vector<PGresult*> one_threaded_res(1000);
        PGresult* one_threaded_res_m[one_threaded_query.size() + 2];
        one_threaded = one_threaded_handle(one_threaded_query, one_threaded_res_m);
        for (auto& i : one_threaded_res_m)
        {
            display_exec_result_sync(i);
            PQclear(i);
        }
    }
    PGconn* conn = PQconnectdb("dbname = pet_project_db");
    auto res1 = PQexec(conn, "TRUNCATE song_table RESTART IDENTITY");
    auto res2 = PQexec(conn,"DELETE FROM log_table; ALTER SEQUENCE log_table_id_seq RESTART WITH 1");
    PQclear(res1);
    PQclear(res2);
    PQfinish(conn);
    std::cout << async << "\n" << one_threaded << std::endl;
    std::cout << getting << std::endl;
    /*int m_i[1000000];
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i != 1000000; ++i)
        m_i[i] = i;
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration<double, std::micro>(stop - start).count();*/
    return 0;
}

double async_handle(const DB_module& db, std::vector<std::string>& query, future_result* async_res)
{
    double async;
    auto start = std::chrono::high_resolution_clock::now();
    size_t i = 0;
    for ( ; i != query.size(); ++i)
    {
        auto res = db.exec_command(query[i].c_str());
        async_res[i] = std::move(res);
    }
    auto res7 = db.exec_command("BEGIN; SELECT FOR SHARE; SELECT * FROM song_table; COMMIT");
    async_res[i] = (std::move(res7));
    auto res8 = db.exec_command("BEGIN; SELECT FOR SHARE; SELECT * FROM log_table; COMMIT");
    ++i;
    async_res[i] = (std::move(res8));
    auto stop = std::chrono::high_resolution_clock::now();
    async = std::chrono::duration<double, std::micro>(stop - start).count();
    return async;
}

double one_threaded_handle(std::vector<std::string>& query, PGresult** one_threaded_res)
{
    double one_threaded;
    PGconn* conn = PQconnectdb(("dbname = pet_project_db"));
    auto start = std::chrono::high_resolution_clock::now();
    size_t i = 0;
    for ( ; i != query.size(); ++i)
    {
        auto res = PQexec(conn, query[i].c_str());
        one_threaded_res[i] = (std::move(res));
    }
    auto res11 = PQexec(conn, "SELECT * FROM song_table");
    one_threaded_res[i] = (std::move(res11));
    auto res12 = PQexec(conn, "SELECT * FROM log_table");
    ++i;
    one_threaded_res[i] = (std::move(res12));
    auto stop = std::chrono::high_resolution_clock::now();
    one_threaded = std::chrono::duration<double, std::micro>(stop - start).count();
    PQfinish(conn);
    return one_threaded;
}

void display_exec_result_sync(PGresult* res)
{
    ExecStatusType res_status = PQresultStatus(res);
    if (res_status == PGRES_TUPLES_OK && PQnfields(res) != 0)
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

/*void display_exec_result_async(PGresult* res)
{
    ExecStatusType res_status = PQresultStatus(res);
    if (res_status == PGRES_TUPLES_OK && PQnfields(res) != 0)
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
}*/

void fill_table(const char* conninfo)
{
    PGconn* conn = PQconnectdb(conninfo);
    for (int i = 0; i != 5; ++i)
    {
        PGresult* res = PQexec(conn, "INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song1', 001)");
        PQclear(res);
    }
    PQfinish(conn);
}
