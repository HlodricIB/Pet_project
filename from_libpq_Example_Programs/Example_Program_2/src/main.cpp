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

#include "libpq-fe.h"

int main(int argc, char *argv[])
{

    return 0;
}
