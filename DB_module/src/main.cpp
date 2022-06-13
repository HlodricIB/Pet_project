#include <chrono>   // To check perfomance gain from current realization of DB_module
#include "libpq-fe.h"
#include "DB_module.h"

#include <thread>

void display_exec_result_sync(PGresult*);
//void display_exec_result_async(PGresult*);
void fill_table(const char*);
void async_handle(const DB_module&, std::vector<std::string>&, future_result*);
double one_threaded_handle(std::vector<std::string>&, PGresult**);

int main()
{
    /*auto res1 = db.exec_command("BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song1', 001); COMMIT");
    auto res2 = db.exec_command("BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song2', 002); COMMIT");
    auto res3 = db.exec_command("BEGIN; SELECT FOR SHARE; SELECT * FROM song_table; COMMIT");
    auto res4 = db.exec_command("BEGIN; SELECT FOR SHARE; SELECT * FROM log_table; COMMIT");*/

    std::vector<std::string> async_query;
    //std::string s = "SELECT FOR SHARE; SELECT * FROM song_table";
    std::string s = "SELECT * FROM song_table";
    for (int i = 0; i != 3; ++i)
    {
        async_query.push_back(s);
    }
    async_query.push_back("SELECT * FROM log_table");
    std::vector<std::string> one_threaded_query;
    s = "SELECT * FROM song_table";
    for (int i = 0; i != 3; ++i)
    {
        one_threaded_query.push_back(s);
    }
    one_threaded_query.push_back("SELECT * FROM log_table");
    double async, one_threaded, showing_async, showing_one_threaded;
    {
        std::cout << "One_threaded" << std::endl;
        fill_table("dbname = pet_project_db");
        //std::vector<PGresult*> one_threaded_res(1000);
        //PGresult* one_threaded_res_m[one_threaded_query.size() + 2];
        PGresult* one_threaded_res_m[one_threaded_query.size()];
        one_threaded = one_threaded_handle(one_threaded_query, one_threaded_res_m);
        auto start = std::chrono::high_resolution_clock::now();
        for (auto& i : one_threaded_res_m)
        {
            display_exec_result_sync(i);
            PQclear(i);
        }
        auto stop = std::chrono::high_resolution_clock::now();
        showing_one_threaded = std::chrono::duration<double, std::micro>(stop - start).count();
    }
    PGconn* conn = PQconnectdb("dbname = pet_project_db");
    auto res1 = PQexec(conn, "TRUNCATE song_table RESTART IDENTITY");
    auto res2 = PQexec(conn,"DELETE FROM log_table; ALTER SEQUENCE log_table_id_seq RESTART WITH 1");
    PQclear(res1);
    PQclear(res2);
    PQfinish(conn);

    {
        std::cout << "Async" << std::endl;
        fill_table("dbname = pet_project_db");
        //std::vector<future_result> async_res;
        //future_result async_res_m[async_query.size() + 2];
        future_result async_res_m[async_query.size()];
        DB_module db{ };
        async_handle(db, async_query, async_res_m);
        PG_result async_results[async_query.size()];
        size_t k = 0;
        auto start = std::chrono::high_resolution_clock::now();
        for (auto& i : async_res_m)
        {
            async_results[k] = std::move(*(i.get()));
            ++k;
        }
        auto stop = std::chrono::high_resolution_clock::now();
        async = std::chrono::duration<double, std::micro>(stop - start).count();
        auto start_showing = std::chrono::high_resolution_clock::now();
        for (auto& i : async_results)
        {
            i.display_exec_result();
        }
        auto stop_showing = std::chrono::high_resolution_clock::now();
        showing_async = std::chrono::duration<double, std::micro>(stop_showing - start_showing).count();
        std::cout << "Truncate and delete" << std::endl;
        db.exec_command("TRUNCATE song_table RESTART IDENTITY");
        db.exec_command("DELETE FROM log_table; ALTER SEQUENCE log_table_id_seq RESTART WITH 1");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Async: " << async << "\n" << "One threaded:" << one_threaded << std::endl;
    std::cout << "Showing async: " << showing_async << "\n" << "Showing one threaded: "
                << showing_one_threaded << std::endl;
    std::cout << "Sum async: " << async + showing_async << "\n" << "Sum one threaded:"
                << one_threaded + showing_one_threaded << std::endl;
    return 0;
}

void async_handle(const DB_module& db, std::vector<std::string>& query, future_result* async_res)
{
    size_t i = 0;
    for ( ; i != query.size(); ++i)
    {
        auto res = db.exec_command(query[i].c_str());
        async_res[i] = std::move(res);
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

double one_threaded_handle(std::vector<std::string>& query, PGresult** one_threaded_res)
{
    double one_threaded;
    PGconn* conn = PQconnectdb(("dbname = pet_project_db"));
    auto start = std::chrono::high_resolution_clock::now();
    size_t i = 0;
    for ( ; i != query.size(); ++i)
    {
        auto start_in = std::chrono::high_resolution_clock::now();
        auto res = PQexec(conn, query[i].c_str());
        auto stop_in = std::chrono::high_resolution_clock::now();
        std::cout << "From main thread: " << std::chrono::duration<double, std::micro>(stop_in - start_in).count() << std::endl;
        one_threaded_res[i] = (std::move(res));
    }
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

void fill_table(const char* conninfo)
{
    PGconn* conn = PQconnectdb(conninfo);
    PGresult* res;
    for (int i = 0; i != 5; ++i)
    {
        res = PQexec(conn, "INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song1', 001)");
        PQclear(res);
    }
    res = PQexec(conn, "INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song2', 001)");
    PQclear(res);
    PQfinish(conn);
}
