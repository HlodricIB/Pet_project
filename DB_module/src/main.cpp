#include "libpq-fe.h"
#include "DB_module.h"

#include <thread>

int main()
{
    DB_module db{ };
    //auto res1 = db.exec_command("BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE; INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song1', 001); COMMIT;");
    auto res2 = db.exec_command("BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song2', 002); COMMIT");
    //std::this_thread::yield();
    auto res3 = db.exec_command("BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; SELECT * FROM song_table; COMMIT");
    //auto res3 = db.exec_command("BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE; SELECT * FROM song_table; COMMIT");
    //auto res4 = db.exec_command("BEGIN; LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE; SELECT * FROM log_table; COMMIT");

    //display_exec_result(std::move(res1));
    display_exec_result(std::move(res2));
    display_exec_result(std::move(res3));
    //display_exec_result(std::move(res4));

    return 0;
}
