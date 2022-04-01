#include "libpq-fe.h"
#include "DB_module.h"

int main()
{
    DB_module db{ };
    db.exec_command("INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song1', 001)");
    db.exec_command("INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, 'song2', 002)");
    db.exec_command("SELECT * FROM song_table");
    db.exec_command("SELECT * FROM log_table");

    return 0;
}
