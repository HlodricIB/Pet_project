/*
    *      Test of the asynchronous notification interface
    *
    * Start this program, then from psql in another window do
    *   NOTIFY TBL2;
    * Repeat four times to get this program to exit.
    *
    * Or, if you want to get fancy, try this:
    * populate a database with the following commands
    * (provided in src/test/examples/testlibpq2.sql):
    *
    *   CREATE SCHEMA TESTLIBPQ2;
    *   SET search_path = TESTLIBPQ2;
    *   CREATE TABLE TBL1 (i int4);
    *   CREATE TABLE TBL2 (i int4);
    *   CREATE RULE r1 AS ON INSERT TO TBL1 DO
    *     (INSERT INTO TBL2 VALUES (new.i); NOTIFY TBL2);
    *
    * Start this program, then from psql do this four times:
    *
    *   INSERT INTO TESTLIBPQ2.TBL1 VALUES (10);
    *
*/

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "libpq-fe.h"
#include "libpq-events.h"

void exit_nicely(PGconn* conn)
{
    PQfinish(conn);
    exit(1);
}

int main(int argc, char *argv[])
{
    const char* conninfo;

    /*
         * If the user supplies a parameter on the command line, use it as the
         * conninfo string; otherwise default to setting dbname=postgres and using
         * environment variables or defaults for all other connection parameters.
    */
    if (argc > 1)
        conninfo = argv[1];
    else
        conninfo = "dbname = testlibpq";

    /* Make a connection to the database */
    PGconn* conn = PQconnectdb(conninfo);

    /* Check to see that the backend connection was successfully made */
    if (PQstatus(conn) != CONNECTION_OK)
    {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
        exit_nicely(conn);
    }

    // Creating tables and a rule.
    PGresult* res = PQexec(conn, "CREATE TABLE TBL1 (i int4); CREATE TABLE TBL2 (i int4); CREATE RULE r1 AS ON INSERT TO TBL1 DO "
                                    "(INSERT INTO TBL2 VALUES (new.i); NOTIFY TBL2)");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Creating tables and a rule failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        exit_nicely(conn);
    }
    PQclear(res);

    /*
         * Issue LISTEN command to enable notifications from the rule's NOTIFY.
    */
    res = PQexec(conn, "LISTEN TBL2");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "LISTEN command failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        exit_nicely(conn);
    }
    PQclear(res);

    /* Quit after four notifies are received. */
    int nnotifies = 0;
    while (nnotifies < 4)
    {
        /*
            * Sleep until something happens on the connection.  We use select(2)
            * to wait for input, but you could also use poll() or similar
            * facilities.
        */
        int sock;
        fd_set input_mask;

        sock = PQsocket(conn);

        if (sock < 0)
                break;  /* shouldn't happen */

        FD_ZERO(&input_mask);
        FD_SET(sock, &input_mask);

        if (select(sock + 1, &input_mask, NULL, NULL, NULL) < 0)
        {
             std::cerr << "select() failed: " << strerror(errno) << std:: endl;
             exit_nicely(conn);
        }

        /* Now check for input */
        PQconsumeInput(conn);
        PGnotify* notify;
        while ((notify = PQnotifies(conn)) != NULL)
        {
            std::cerr << "ASYNC NOTIFY of " << notify->relname << " received from backend PID " << notify->be_pid << std::endl;
            PQfreemem(notify);
            nnotifies++;
            PQconsumeInput(conn);
        }
    }

    std::cerr << "Done\n";
    /* close the connection to the database and cleanup */
    PQfinish(conn);
    return 0;
}
