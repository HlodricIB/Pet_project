#ifndef DB_MODULE_H
#define DB_MODULE_H

#include <iostream>
#include <queue>
#include <memory>
#include "libpq-fe.h"
#include "parser.h"

class connection_pool
{
private:
    std::queue<PGconn*> conns;
    void connection_establishing();
public:
    connection_pool() { }
    connection_pool(const Parser_DB&);
    ~connection_pool();
    PGconn* pull_connection();
    void push_connection(PGconn*);


    //For standalone module only:
    connection_pool(const char* conninfo = "dbname = pet_project_db");

};

class DB_module
{
private:
    connection_pool* conns;
    void exit_nicely() { PQfinish(conn); exit(1); };
public:
    DB_module(const Parser_DB&);
    ~DB_module();
    void exec_command(const char*);

    //For standalone module only:
    DB_module(const char* conninfo = "dbname = pet_project_db") { conn = PQconnectdb(conninfo); }
};



#endif // DB_MODULE_H
